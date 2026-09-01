#pragma once

#include <cstdint>

namespace midi {

class PicoSwitches {
 public:
  static constexpr std::uint8_t mask_for_index(unsigned index) { return index < 4 ? static_cast<std::uint8_t>(1u << index) : 0; }

  void initialize();
  [[nodiscard]] std::uint8_t read_mask() const;
};

}  // namespace midi
