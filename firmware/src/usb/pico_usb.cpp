#include "pico_usb.hpp"

#include <algorithm>
#include <array>

namespace midi::usb {

bool PicoUsbPort::enqueue_cdc(std::span<const std::byte> bytes) {
  if (bytes.empty() || bytes.size() > CdcFrameCapacity || queue_size_ == queue_.size()) return false;
  auto& pending = queue_[queue_write_];
  std::copy(bytes.begin(), bytes.end(), pending.bytes.begin());
  pending.length = bytes.size();
  pending.offset = 0;
  queue_write_ = (queue_write_ + 1) % queue_.size();
  ++queue_size_;
  return true;
}

void PicoUsbPort::flush_cdc() {
  while (queue_size_ != 0 && api_.mounted()) {
    auto& pending = queue_[queue_read_];
    if (pending.offset == pending.length) {
      pending = {};
      queue_read_ = (queue_read_ + 1) % queue_.size();
      --queue_size_;
      continue;
    }
    const auto available = api_.cdc_write_available();
    if (available == 0) break;
    const auto remaining = pending.length - pending.offset;
    const auto count = std::min(available, remaining);
    const auto written = api_.cdc_write(std::span<const std::byte>(pending.bytes.data() + pending.offset, count));
    if (written == 0 || written > count) break;
    pending.offset += written;
    api_.cdc_flush();
  }
}

void PicoUsbPort::service(ProtocolDispatcher& dispatcher) {
  api_.task();
  flush_cdc();

  std::array<std::byte, 256> input{};
  for (unsigned pass = 0; pass < 4 && api_.cdc_available() != 0; ++pass) {
    const auto count = api_.cdc_read(input);
    if (count == 0) break;
    dispatcher.receive(std::span<const std::byte>(input.data(), count));
  }
  flush_cdc();
}

bool PicoUsbPort::write_midi(std::span<const std::uint8_t> packet) {
  if (!api_.midi_mounted() || packet.size() != 4) return false;
  return api_.midi_write(packet);
}

}  // namespace midi::usb
