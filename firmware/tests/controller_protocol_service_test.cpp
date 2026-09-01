#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "app/controller_protocol_service.hpp"
#include "app/pedal_runtime.hpp"
#include "core/config_store.hpp"
#include "core/crc32.hpp"
#include "core/expression.hpp"

namespace {
class FakeFlash final : public midi::FlashPort {
 public:
  static constexpr std::size_t Size = 4 * 1024 * 1024;
  FakeFlash() : bytes(Size, std::byte{0xff}) {}

  bool erase(std::uint32_t offset, std::size_t length) override {
    if (should_fail()) return false;
    if (offset + length > bytes.size()) return false;
    erase_lengths.push_back(length);
    std::fill(bytes.begin() + offset, bytes.begin() + offset + length, std::byte{0xff});
    return true;
  }
  bool program(std::uint32_t offset, std::span<const std::byte> data) override {
    if (should_fail()) return false;
    if (offset + data.size() > bytes.size()) return false;
    for (std::size_t index = 0; index < data.size(); ++index) bytes[offset + index] &= data[index];
    return true;
  }
  void read(std::uint32_t offset, std::span<std::byte> output) const override {
    if (offset + output.size() > bytes.size()) { std::fill(output.begin(), output.end(), std::byte{0}); return; }
    std::copy_n(bytes.begin() + offset, output.size(), output.begin());
  }
  const std::byte* mapped(std::uint32_t offset, std::size_t length) const override {
    return offset + length <= bytes.size() ? bytes.data() + offset : nullptr;
  }

  void fail_after(std::size_t operation) { fail_at = operation; operations = 0; }

  std::vector<std::byte> bytes;
  std::vector<std::size_t> erase_lengths;

 private:
  bool should_fail() { return fail_at.has_value() && operations++ >= *fail_at; }
  std::optional<std::size_t> fail_at;
  std::size_t operations{};
};

class FakeExpressionInput final : public midi::ExpressionSampleInput {
 public:
  std::uint16_t value{2048};
  std::uint16_t read_adc() const override { return value; }
};

class RuntimeConfig final : public midi::RuntimeConfigSource {
 public:
  midi::RuntimeConfigSnapshot status() const override { return {true, 1, 1}; }
  bool load_bank(std::uint8_t index, midi::BankConfig& output) const override {
    if (index != 0) return false;
    output = bank;
    return true;
  }

 private:
  midi::BankConfig bank{};
};

class RuntimeSwitches final : public midi::RuntimeSwitchSource {
 public:
  std::uint8_t mask{};
  std::uint8_t read_mask() const override { return mask; }
};

class RuntimeExpression final : public midi::RuntimeExpressionSource {
 public:
  std::uint16_t read_adc() const override { return 2048; }
};

class RuntimeMidi final : public midi::MidiPort {
 public:
  bool enqueue(midi::Destination, midi::MidiMessage) override { return true; }
};

class RuntimeRelays final : public midi::RelayPort {
 public:
  void set(std::uint8_t, bool) override {}
};

class RuntimeUsbPort final : public midi::usb::UsbPort {
 public:
  bool mounted() const override { return true; }
  bool write_cdc(std::span<const std::byte>) override { return true; }
  bool write_midi(std::span<const std::uint8_t>) override { return true; }
};

class RuntimeDisplay final : public midi::DisplayPort {
 public:
  void present(const midi::LiveView&) override {}
};

class FakeLiveAction final : public midi::LiveActionState {
 public:
  bool active{};
  bool live_action_active() const override { return active; }
};

class RecordingSink final : public midi::usb::ProtocolResponseSink {
 public:
  bool write_frame(std::span<const std::byte> frame) override {
    frames.emplace_back(frame.begin(), frame.end());
    return true;
  }

  std::vector<std::vector<std::byte>> frames;
};

std::vector<std::byte> fixture() {
  std::ifstream stream(std::string(MIDI_PEDAL_SOURCE_DIR) + "/protocol/fixtures/bin/minimal-valid.bin", std::ios::binary);
  const std::vector<char> raw{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
  std::vector<std::byte> output(raw.size());
  for (std::size_t index = 0; index < raw.size(); ++index) output[index] = static_cast<std::byte>(static_cast<unsigned char>(raw[index]));
  return output;
}

std::uint32_t u32(std::span<const std::byte> bytes, std::size_t at) {
  return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[at]) |
    (std::to_integer<std::uint8_t>(bytes[at + 1]) << 8u) |
    (std::to_integer<std::uint8_t>(bytes[at + 2]) << 16u) |
    (std::to_integer<std::uint8_t>(bytes[at + 3]) << 24u));
}

std::vector<std::byte> request(std::uint32_t request_id, midi::usb::Command command,
                               std::span<const std::byte> payload) {
  std::vector<std::byte> bytes(18 + payload.size() + 4);
  bytes[0] = std::byte{'M'};
  bytes[1] = std::byte{'P'};
  bytes[2] = std::byte{'C'};
  bytes[3] = std::byte{'F'};
  const auto put16 = [&bytes](std::size_t at, std::uint16_t value) {
    bytes[at] = static_cast<std::byte>(value & 0xffu);
    bytes[at + 1] = static_cast<std::byte>(value >> 8u);
  };
  const auto put32 = [&bytes](std::size_t at, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) bytes[at + shift / 8] = static_cast<std::byte>(value >> shift);
  };
  put16(4, 1);
  put32(6, request_id);
  put16(10, static_cast<std::uint16_t>(command));
  put16(12, 0);
  put32(14, static_cast<std::uint32_t>(payload.size()));
  std::copy(payload.begin(), payload.end(), bytes.begin() + 18);
  put32(18 + payload.size(), midi::crc32(std::span<const std::byte>(bytes.data(), 18 + payload.size())));
  return bytes;
}

void activate(midi::ConfigStore& store, std::span<const std::byte> image) {
  ASSERT_TRUE(store.begin_upload(static_cast<std::uint32_t>(image.size()), 1, u32(image, 28)));
  for (std::size_t offset = 0; offset < image.size(); offset += 1024) {
    ASSERT_TRUE(store.write_chunk(static_cast<std::uint32_t>(offset), image.subspan(offset, std::min<std::size_t>(1024, image.size() - offset))));
  }
  ASSERT_TRUE(store.verify_upload());
  ASSERT_TRUE(store.activate_upload());
}
}  // namespace

TEST(ControllerProtocolService, ReportsCapabilitiesAndActiveMetadata) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  const auto image = fixture();
  activate(store, image);
  FakeExpressionInput input;
  midi::ExpressionProcessor expression({0, 4095});
  midi::ControllerProtocolService service(store, input, expression);

  std::array<std::byte, 4095> output{};
  std::size_t size = 0;
  ASSERT_EQ(service.capabilities(output, size), midi::usb::ServiceError::None);
  const std::string capabilities(reinterpret_cast<const char*>(output.data()), size);
  EXPECT_NE(capabilities.find("MIDI_PEDAL_PICO2"), std::string::npos);
  EXPECT_NE(capabilities.find("ST7796S"), std::string::npos);

  ASSERT_EQ(service.config_info(output, size), midi::usb::ServiceError::None);
  const std::string metadata(reinterpret_cast<const char*>(output.data()), size);
  EXPECT_NE(metadata.find("\"sequence\":1"), std::string::npos);
  EXPECT_NE(metadata.find("\"bankCount\":1"), std::string::npos);
}

TEST(ControllerProtocolService, ReadsValidatedBankRecordAndSamplesExpression) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  const auto image = fixture();
  activate(store, image);
  FakeExpressionInput input;
  midi::ExpressionProcessor expression({0, 4095});
  midi::ControllerProtocolService service(store, input, expression);

  std::array<std::byte, 4095> output{};
  std::size_t size = 0;
  ASSERT_EQ(service.read_config(0, output, size), midi::usb::ServiceError::None);
  EXPECT_EQ(size, u32(output, 0));
  EXPECT_GT(size, 32U);

  std::uint16_t value = 0;
  ASSERT_EQ(service.expression_sample(value), midi::usb::ServiceError::None);
  EXPECT_EQ(value, 2048U);
  EXPECT_EQ(service.set_expression_calibration(3000, 2000), midi::usb::ServiceError::InvalidConfiguration);
  EXPECT_EQ(service.set_expression_calibration(100, 2000), midi::usb::ServiceError::None);
}

TEST(ControllerProtocolService, RejectsUploadWhenConfigStoreRejectsIt) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  FakeExpressionInput input;
  midi::ExpressionProcessor expression({0, 4095});
  midi::ControllerProtocolService service(store, input, expression);
  EXPECT_EQ(service.begin_upload(0, 1, 0), midi::usb::ServiceError::InvalidConfiguration);
  EXPECT_EQ(service.verify_upload(), midi::usb::ServiceError::VerifyFailed);
  EXPECT_EQ(service.activate_upload(), midi::usb::ServiceError::InvalidState);
}

TEST(ControllerProtocolService, FactoryResetValidatesAndActivatesEmbeddedImage) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  FakeExpressionInput input;
  midi::ExpressionProcessor expression({0, 4095});
  const auto image = fixture();
  midi::ControllerProtocolService service(store, input, expression, image);

  EXPECT_EQ(service.factory_empty_reset(), midi::usb::ServiceError::None);
  const auto info = store.active_info();
  ASSERT_TRUE(info.has_value());
  EXPECT_EQ(info->image_size, image.size());
  EXPECT_EQ(info->image_crc32, u32(image, 28));
  EXPECT_EQ(info->bank_count, 128U);
  ASSERT_FALSE(flash.erase_lengths.empty());
  EXPECT_TRUE(std::all_of(flash.erase_lengths.begin(), flash.erase_lengths.end(),
                          [](std::size_t length) { return length == midi::ConfigStore::MetadataSectorSize; }));
}

TEST(ControllerProtocolService, RefusesMutationsWhileALiveActionIsExecuting) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  FakeExpressionInput input;
  midi::ExpressionProcessor expression({0, 4095});
  const auto image = fixture();
  midi::ControllerProtocolService service(store, input, expression, image);
  FakeLiveAction action;
  service.set_live_action_state(&action);
  action.active = true;
  const std::array<std::byte, 1> chunk{std::byte{0}};

  EXPECT_EQ(service.begin_upload(100, 1, 0), midi::usb::ServiceError::Busy);
  EXPECT_EQ(service.write_chunk(0, chunk), midi::usb::ServiceError::Busy);
  EXPECT_EQ(service.verify_upload(), midi::usb::ServiceError::Busy);
  EXPECT_EQ(service.activate_upload(), midi::usb::ServiceError::Busy);
  EXPECT_EQ(service.factory_empty_reset(), midi::usb::ServiceError::Busy);
  EXPECT_EQ(service.set_expression_calibration(100, 3900), midi::usb::ServiceError::Busy);
  EXPECT_EQ(expression.calibration().heel, 0U);
  EXPECT_EQ(expression.calibration().toe, 4095U);
}

TEST(ControllerProtocolService, DefersMutationsUntilAfterTheProductionControlPhase) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  FakeExpressionInput input;
  midi::ExpressionProcessor expression({0, 4095});
  midi::ControllerProtocolService service(store, input, expression);
  RuntimeConfig config;
  RuntimeSwitches switches;
  RuntimeExpression expression_input;
  RuntimeMidi midi;
  RuntimeRelays relays;
  RuntimeUsbPort usb_port;
  midi::usb::UsbTransport usb_midi(usb_port);
  RuntimeDisplay display;
  midi::PedalRuntime runtime(config, switches, expression_input, midi, relays, usb_midi, usb_port, display, expression);
  service.set_live_action_state(&runtime);
  RecordingSink sink;
  midi::usb::ProtocolDispatcher dispatcher(service, sink);
  const std::array begin_payload{std::byte{64}, std::byte{0}, std::byte{0}, std::byte{0},
                                 std::byte{1}, std::byte{0}, std::byte{0}, std::byte{0},
                                 std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};

  ASSERT_TRUE(runtime.initialize());
  switches.mask = 0x01;
  runtime.tick(0);
  runtime.tick(35);
  dispatcher.receive(request(1, midi::usb::Command::BeginUpload, begin_payload));

  ASSERT_EQ(sink.frames.size(), 1U);
  EXPECT_EQ(sink.frames.back()[18], std::byte{1});
  const std::string busy(reinterpret_cast<const char*>(sink.frames.back().data() + 19), sink.frames.back().size() - 23);
  EXPECT_NE(busy.find("BUSY"), std::string::npos);

  runtime.finish_control_phase();
  dispatcher.receive(request(2, midi::usb::Command::BeginUpload, begin_payload));
  ASSERT_EQ(sink.frames.size(), 2U);
  EXPECT_EQ(sink.frames.back()[18], std::byte{0});
}

TEST(ControllerProtocolService, CommitsCalibrationBeforeChangingTheExpressionProcessor) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  FakeExpressionInput input;
  midi::ExpressionProcessor expression({0, 4095});
  midi::ControllerProtocolService service(store, input, expression);

  ASSERT_EQ(service.set_expression_calibration(100, 3900), midi::usb::ServiceError::None);
  midi::ConfigStore rebooted(flash);
  EXPECT_EQ(rebooted.expression_calibration().heel, 100U);
  EXPECT_EQ(rebooted.expression_calibration().toe, 3900U);
}

TEST(ControllerProtocolService, DoesNotChangeExpressionCalibrationWhenPersistenceFails) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  FakeExpressionInput input;
  midi::ExpressionProcessor expression({0, 4095});
  midi::ControllerProtocolService service(store, input, expression);

  flash.fail_after(0);
  EXPECT_EQ(service.set_expression_calibration(100, 3900), midi::usb::ServiceError::InvalidConfiguration);
  EXPECT_EQ(expression.calibration().heel, 0U);
  EXPECT_EQ(expression.calibration().toe, 4095U);
}
