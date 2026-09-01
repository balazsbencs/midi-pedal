#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "usb/pico_usb.hpp"

namespace {
class FakeApi final : public midi::usb::UsbDeviceApi {
 public:
  bool mounted_state{true};
  std::size_t write_limit{64};
  std::vector<std::byte> received;
  std::vector<std::byte> cdc_output;
  std::vector<std::uint8_t> midi_output;
  std::size_t task_calls{};

  bool mounted() const override { return mounted_state; }
  std::size_t cdc_available() const override { return received.size(); }
  std::size_t cdc_read(std::span<std::byte> output) override {
    const auto count = std::min(output.size(), received.size());
    std::copy_n(received.begin(), count, output.begin());
    received.erase(received.begin(), received.begin() + static_cast<std::ptrdiff_t>(count));
    return count;
  }
  std::size_t cdc_write_available() const override { return mounted_state ? write_limit : 0; }
  std::size_t cdc_write(std::span<const std::byte> bytes) override {
    const auto count = std::min(write_limit, bytes.size());
    cdc_output.insert(cdc_output.end(), bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(count));
    return count;
  }
  void cdc_flush() override {}
  bool midi_mounted() const override { return mounted_state; }
  bool midi_write(std::span<const std::uint8_t> packet) override {
    midi_output.insert(midi_output.end(), packet.begin(), packet.end());
    return true;
  }
  bool initialize() override { return true; }
  void task() override { ++task_calls; }
};

class NoopService final : public midi::usb::ProtocolService {
 public:
  midi::usb::ServiceError capabilities(std::span<std::byte>, std::size_t& size) override { size = 0; return midi::usb::ServiceError::None; }
  midi::usb::ServiceError config_info(std::span<std::byte>, std::size_t& size) override { size = 0; return midi::usb::ServiceError::None; }
  midi::usb::ServiceError read_config(std::uint8_t, std::span<std::byte>, std::size_t& size) override { size = 0; return midi::usb::ServiceError::None; }
  midi::usb::ServiceError begin_upload(std::uint32_t, std::uint32_t, std::uint32_t) override { return midi::usb::ServiceError::None; }
  midi::usb::ServiceError write_chunk(std::uint32_t, std::span<const std::byte>) override { return midi::usb::ServiceError::None; }
  midi::usb::ServiceError verify_upload() override { return midi::usb::ServiceError::None; }
  midi::usb::ServiceError activate_upload() override { return midi::usb::ServiceError::None; }
  midi::usb::ServiceError expression_sample(std::uint16_t& value) override { value = 0; return midi::usb::ServiceError::None; }
  midi::usb::ServiceError set_expression_calibration(std::uint16_t, std::uint16_t) override { return midi::usb::ServiceError::None; }
  midi::usb::ServiceError factory_empty_reset() override { return midi::usb::ServiceError::None; }
};
}  // namespace

TEST(PicoUsbPort, FlushesQueuedCdcFramesAcrossEndpointCapacity) {
  FakeApi api;
  api.write_limit = 2;
  midi::usb::PicoUsbPort port(api);
  NoopService service;
  midi::usb::ProtocolDispatcher dispatcher(service, port);
  const std::array frame{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}};

  ASSERT_TRUE(port.write_frame(frame));
  port.service(dispatcher);

  EXPECT_EQ(api.cdc_output, std::vector<std::byte>(frame.begin(), frame.end()));
  EXPECT_EQ(api.task_calls, 1U);
}

TEST(PicoUsbPort, SendsMidiOnlyWhenMounted) {
  FakeApi api;
  midi::usb::PicoUsbPort port(api);
  const std::array packet{std::uint8_t{0x0b}, std::uint8_t{0xb0}, std::uint8_t{1}, std::uint8_t{127}};
  EXPECT_TRUE(port.write_midi(packet));
  EXPECT_EQ(api.midi_output, std::vector<std::uint8_t>(packet.begin(), packet.end()));

  api.mounted_state = false;
  EXPECT_FALSE(port.write_midi(packet));
}

TEST(PicoUsbPort, ReportsQueuePressureWithoutBlocking) {
  FakeApi api;
  midi::usb::PicoUsbPort port(api);
  NoopService service;
  midi::usb::ProtocolDispatcher dispatcher(service, port);
  std::array frame{std::byte{0x55}};
  for (std::size_t index = 0; index < midi::usb::PicoUsbPort::CdcQueueCapacity; ++index) EXPECT_TRUE(port.write_frame(frame));
  EXPECT_FALSE(port.write_frame(frame));
  port.service(dispatcher);
  EXPECT_EQ(api.task_calls, 1U);
}
