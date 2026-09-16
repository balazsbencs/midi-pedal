#pragma once

#include <cstdint>
#include <span>

namespace midi::display {

enum class DeckFontRole : std::uint8_t {
  Switch,
  Title,
  CompactTitle,
  Bank,
  Label,
  Footer,
};

struct DeckGlyph {
  std::uint32_t offset{};
  std::uint16_t width{};
  std::uint16_t height{};
  std::int8_t bearingX{};
  std::int8_t bearingY{};
  std::uint8_t advance{};
};

struct DeckFont {
  std::span<const DeckGlyph> glyphs;
  std::span<const std::uint8_t> pixels;
  std::uint8_t firstCharacter{};
  std::uint8_t lastCharacter{};
  std::uint8_t lineHeight{};
};

const DeckFont& deck_font(DeckFontRole role);

}  // namespace midi::display
