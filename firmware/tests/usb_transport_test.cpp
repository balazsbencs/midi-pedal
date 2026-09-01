#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include "usb/usb_transport.hpp"

namespace {
struct FakeUsb final : midi::usb::UsbPort {
  bool connected{true};
  std::array<std::uint8_t, 4> packet{};
  bool mounted() const override { return connected; }
  bool write_cdc(std::span<const std::byte>) override { return true; }
  bool write_midi(std::span<const std::uint8_t> bytes) override {
    std::copy(bytes.begin(), bytes.end(), packet.begin());
    return true;
  }
};
}  // namespace

TEST(UsbTransport, EncodesControlChangeAsUsbMidiPacket) {
  FakeUsb port;
  midi::usb::UsbTransport transport(port);
  const midi::MidiMessage message{{0xb0, 17, 127}, 3};
  ASSERT_TRUE(transport.send_midi(midi::Destination::Usb, message));
  EXPECT_EQ(port.packet, (std::array<std::uint8_t, 4>{0x0b, 0xb0, 17, 127}));
}

TEST(UsbTransport, DropsUnavailableUsbWithoutBlockingAction) {
  FakeUsb port;
  port.connected = false;
  midi::usb::UsbTransport transport(port);
  EXPECT_TRUE(transport.send_midi(midi::Destination::Both, {{0xc0, 5, 0}, 2}));
  EXPECT_EQ(transport.dropped_midi(), 1U);
}
