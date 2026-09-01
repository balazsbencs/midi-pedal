#pragma once

#include <array>
#include <cstddef>

#include "config_types.hpp"

namespace midi {

template <std::size_t Capacity>
class MidiQueue {
  static_assert(Capacity > 0, "MIDI queue capacity must be positive");

 public:
  [[nodiscard]] bool push(MidiMessage message) {
    if (size_ == Capacity) return false;
    messages_[write_] = message;
    write_ = (write_ + 1) % Capacity;
    ++size_;
    return true;
  }

  [[nodiscard]] bool pop(MidiMessage& message) {
    if (size_ == 0) return false;
    message = messages_[read_];
    read_ = (read_ + 1) % Capacity;
    --size_;
    return true;
  }

  [[nodiscard]] const MidiMessage* front() const { return size_ == 0 ? nullptr : &messages_[read_]; }
  [[nodiscard]] std::size_t size() const { return size_; }
  [[nodiscard]] constexpr std::size_t capacity() const { return Capacity; }
  [[nodiscard]] bool empty() const { return size_ == 0; }
  [[nodiscard]] bool full() const { return size_ == Capacity; }
  void clear() { read_ = write_ = size_ = 0; }

 private:
  std::array<MidiMessage, Capacity> messages_{};
  std::size_t read_{};
  std::size_t write_{};
  std::size_t size_{};
};

}  // namespace midi
