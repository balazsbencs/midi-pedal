#pragma once

#include <array>
#include <cstdint>

namespace midi::display {

using Glyph = std::array<std::uint8_t, 7>;

Glyph glyph_for(char character);

}  // namespace midi::display

