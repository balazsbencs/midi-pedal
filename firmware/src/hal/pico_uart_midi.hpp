#pragma once

#include <cstdint>

#include "../core/midi_queue.hpp"
#include "ports.hpp"

namespace midi {

class PicoUartMidi final : public MidiPort {
 public:
  static constexpr std::uint32_t BaudRate = 31250;

  void initialize();
  bool enqueue(Destination destination, MidiMessage message) override;
  void service();
  [[nodiscard]] std::size_t queued() const { return queue_.size(); }

 private:
  MidiQueue<64> queue_;
};

}  // namespace midi
