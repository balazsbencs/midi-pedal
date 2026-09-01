#pragma once

#include <cstdint>

namespace midi {

class PicoExpressionAdc {
 public:
  static constexpr std::uint8_t AdcInput = 0;

  void initialize();
  [[nodiscard]] std::uint16_t read() const;
};

}  // namespace midi
