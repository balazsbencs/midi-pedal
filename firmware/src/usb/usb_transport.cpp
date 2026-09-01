#include "usb_transport.hpp"

namespace midi::usb {

std::array<std::uint8_t, 4> UsbTransport::midi_packet(MidiMessage message) {
  std::uint8_t cin = message.length == 2 ? 0x0c : 0x0b;
  return {cin, message.bytes[0], message.bytes[1], message.length > 2 ? message.bytes[2] : 0};
}

bool UsbTransport::send_frame(std::span<const std::byte> frame) {
  if (!port_.mounted()) return false;
  return port_.write_cdc(frame);
}

bool UsbTransport::send_midi(Destination destination, MidiMessage message) {
  if (destination == Destination::Trs) return true;
  if (!port_.mounted()) {
    ++dropped_midi_;
    return true;
  }
  const auto packet = midi_packet(message);
  if (!port_.write_midi(packet)) ++dropped_midi_;
  // USB unavailability is a diagnosed, non-fatal output condition; it must not
  // prevent TRS output or a preset transition.
  return true;
}

}  // namespace midi::usb
