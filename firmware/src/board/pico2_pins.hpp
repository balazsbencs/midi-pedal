#pragma once

#include <array>
#include <cstdint>

namespace midi::board {

inline constexpr std::uint8_t SwitchA = 2;
inline constexpr std::uint8_t SwitchB = 3;
inline constexpr std::uint8_t SwitchC = 4;
inline constexpr std::uint8_t SwitchD = 5;
inline constexpr std::uint8_t MidiTx = 8;
inline constexpr std::uint8_t Relay1 = 10;
inline constexpr std::uint8_t Relay2 = 11;
inline constexpr std::uint8_t DisplayCs = 17;
inline constexpr std::uint8_t DisplaySck = 18;
inline constexpr std::uint8_t DisplayMosi = 19;
inline constexpr std::uint8_t DisplayDc = 20;
inline constexpr std::uint8_t DisplayReset = 21;
inline constexpr std::uint8_t ExpressionAdc = 26;

constexpr bool unique_pins() {
  constexpr std::array pins{SwitchA, SwitchB, SwitchC, SwitchD, MidiTx, Relay1, Relay2, DisplayCs, DisplaySck, DisplayMosi, DisplayDc, DisplayReset, ExpressionAdc};
  for (std::size_t left = 0; left < pins.size(); ++left) for (std::size_t right = left + 1; right < pins.size(); ++right) if (pins[left] == pins[right]) return false;
  return true;
}

static_assert(unique_pins(), "every assigned Pico 2 GPIO must be unique");
static_assert(ExpressionAdc == 26, "expression input must remain on ADC0/GP26");

}  // namespace midi::board
