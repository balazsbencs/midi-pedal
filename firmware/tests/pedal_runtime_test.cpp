#include <array>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "app/pedal_runtime.hpp"
#include "usb/pico_usb.hpp"

namespace {
class FakeConfig final : public midi::RuntimeConfigSource {
 public:
  midi::RuntimeConfigSnapshot snapshot{true, 7, 2};
  std::array<midi::BankConfig, 2> banks{};

  midi::RuntimeConfigSnapshot status() const override { return snapshot; }

  bool load_bank(std::uint8_t index, midi::BankConfig& output) const override {
    if (index >= snapshot.bank_count || index >= banks.size()) return false;
    output = banks[index];
    return true;
  }
};

class FakeSwitches final : public midi::RuntimeSwitchSource {
 public:
  std::uint8_t mask{};
  std::uint8_t read_mask() const override { return mask; }
};

class FakeExpression final : public midi::RuntimeExpressionSource {
 public:
  std::uint16_t value{2048};
  std::uint16_t read_adc() const override { return value; }
};

class FakeMidi final : public midi::MidiPort {
 public:
  struct Sent { midi::Destination destination{}; midi::MidiMessage message{}; };
  std::vector<Sent> sent;
  bool enqueue(midi::Destination destination, midi::MidiMessage message) override {
    sent.push_back({destination, message});
    return true;
  }
};

class FakeRelays final : public midi::RelayPort {
 public:
  std::array<bool, 2> closed{};
  void set(std::uint8_t contact, bool value) override {
    if (contact >= 1 && contact <= 2) closed[contact - 1] = value;
  }
};

class FakeUsb final : public midi::usb::UsbDeviceApi {
 public:
  bool mounted_state{true};
  std::vector<std::uint8_t> midi;

  bool mounted() const override { return mounted_state; }
  std::size_t cdc_available() const override { return 0; }
  std::size_t cdc_read(std::span<std::byte>) override { return 0; }
  std::size_t cdc_write_available() const override { return 0; }
  std::size_t cdc_write(std::span<const std::byte>) override { return 0; }
  void cdc_flush() override {}
  bool midi_mounted() const override { return mounted_state; }
  bool midi_write(std::span<const std::uint8_t> packet) override {
    midi.insert(midi.end(), packet.begin(), packet.end());
    return true;
  }
  bool initialize() override { return true; }
  void task() override {}
};

class FakeDisplay final : public midi::DisplayPort {
 public:
  std::vector<midi::LiveView> views;
  void present(const midi::LiveView& view) override { views.push_back(view); }
};

midi::BankConfig bank(std::uint32_t bank_id, std::uint32_t preset_id) {
  midi::BankConfig output{};
  output.id = bank_id;
  output.pages[0].id = bank_id * 10;
  output.pages[0].presets[0].id = preset_id;
  output.pages[0].presets[0].slotCount = 1;
  output.pages[0].presets[0].slots[0] = {
      1, midi::Trigger::Press, midi::PositionFilter::Position1,
      {midi::MessageKind::Cc, midi::Destination::Both, 1, 17, 127, 0, 0, 0}};
  output.pages[0].presets[0].toggleOn = static_cast<std::uint8_t>(midi::Trigger::Press);
  return output;
}
}  // namespace

TEST(PedalRuntime, RoutesBothDestinationAndUpdatesToggleView) {
  FakeConfig config;
  config.banks[0] = bank(10, 100);
  config.banks[1] = bank(20, 200);
  FakeSwitches switches;
  FakeExpression expression_input;
  FakeMidi trs;
  FakeRelays relays;
  FakeUsb usb_api;
  midi::usb::PicoUsbPort usb_port(usb_api);
  midi::usb::UsbTransport usb_transport(usb_port);
  FakeDisplay display;
  midi::ExpressionProcessor expression({0, 4095});
  midi::PedalRuntime runtime(config, switches, expression_input, trs, relays, usb_transport, usb_port, display, expression);

  ASSERT_TRUE(runtime.initialize());
  ASSERT_FALSE(display.views.empty());
  EXPECT_EQ(display.views.back().bank, 1U);
  EXPECT_EQ(display.views.back().positions[0], 1U);

  switches.mask = 0x01;
  runtime.tick(0);
  runtime.tick(35);

  ASSERT_EQ(trs.sent.size(), 1U);
  EXPECT_EQ(trs.sent[0].destination, midi::Destination::Both);
  ASSERT_EQ(usb_api.midi.size(), 4U);
  EXPECT_EQ(usb_api.midi[0], 0x0bU);
  EXPECT_EQ(usb_api.midi[1], 0xb0U);
  EXPECT_EQ(usb_api.midi[2], 17U);
  EXPECT_EQ(usb_api.midi[3], 127U);
  EXPECT_EQ(display.views.back().positions[0], 2U);
}

TEST(PedalRuntime, PageChordWrapsAndReloadsExpressionAssignment) {
  FakeConfig config;
  config.banks[0] = bank(10, 100);
  config.banks[1] = bank(20, 200);
  FakeSwitches switches;
  FakeExpression expression_input;
  FakeMidi trs;
  FakeRelays relays;
  FakeUsb usb_api;
  midi::usb::PicoUsbPort usb_port(usb_api);
  midi::usb::UsbTransport usb_transport(usb_port);
  FakeDisplay display;
  midi::ExpressionProcessor expression({0, 4095});
  midi::PedalRuntime runtime(config, switches, expression_input, trs, relays, usb_transport, usb_port, display, expression);

  ASSERT_TRUE(runtime.initialize());
  switches.mask = 0x03;  // A+B = page down.
  runtime.tick(0);
  runtime.tick(35);

  ASSERT_FALSE(display.views.empty());
  EXPECT_EQ(display.views.back().page, 4U);
  EXPECT_EQ(display.views.back().bank, 1U);
}

TEST(PedalRuntime, MissingImageStartsSafeEmptyWithoutBlockingTheDevice) {
  FakeConfig config;
  config.snapshot = {false, 0, 0};
  FakeSwitches switches;
  FakeExpression expression_input;
  FakeMidi trs;
  FakeRelays relays;
  FakeUsb usb_api;
  midi::usb::PicoUsbPort usb_port(usb_api);
  midi::usb::UsbTransport usb_transport(usb_port);
  FakeDisplay display;
  midi::ExpressionProcessor expression({0, 4095});
  midi::PedalRuntime runtime(config, switches, expression_input, trs, relays, usb_transport, usb_port, display, expression);

  EXPECT_FALSE(runtime.initialize());
  ASSERT_FALSE(display.views.empty());
  EXPECT_FALSE(display.views.back().configurationError);
  EXPECT_FALSE(display.views.back().expressionAvailable);
  EXPECT_EQ(display.views.back().positions, (std::array<std::uint8_t, 4>{1, 1, 1, 1}));
}
