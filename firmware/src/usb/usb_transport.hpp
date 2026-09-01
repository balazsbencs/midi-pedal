#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "../core/action_engine.hpp"
#include "../core/frame_decoder.hpp"

namespace midi::usb {

class UsbPort {
 public:
  virtual ~UsbPort() = default;
  [[nodiscard]] virtual bool mounted() const = 0;
  [[nodiscard]] virtual bool write_cdc(std::span<const std::byte> bytes) = 0;
  [[nodiscard]] virtual bool write_midi(std::span<const std::uint8_t> packet) = 0;
};

class UsbTransport final : public ActionSink {
 public:
  explicit UsbTransport(UsbPort& port) : port_(port) {}

  void receive_cdc(std::span<const std::byte> bytes, FrameSink& sink) { decoder_.feed(bytes, sink); }
  [[nodiscard]] bool send_frame(std::span<const std::byte> frame);
  [[nodiscard]] bool send_midi(Destination destination, MidiMessage message) override;
  [[nodiscard]] bool relay(RelayCommand) override { return true; }
  [[nodiscard]] bool navigate(NavigationCommand) override { return true; }
  [[nodiscard]] std::uint32_t dropped_midi() const { return dropped_midi_; }

 private:
  static std::array<std::uint8_t, 4> midi_packet(MidiMessage message);

  UsbPort& port_;
  FrameDecoder decoder_;
  std::uint32_t dropped_midi_{};
};

}  // namespace midi::usb
