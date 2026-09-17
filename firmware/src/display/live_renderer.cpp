/*
THESIS: Chromatic Deck turns the fixed A–D switch map into four memorable color surfaces; it refuses a generic outlined dashboard.
OWN-WORLD: Blue-black stage surfaces, saturated preset rails and fills, condensed display type, hyperlegible status type, flat tonal depth.
STORY: Read the bank, identify each switch assignment, recognize every toggle state, then confirm expression and connection status.
FIRST VIEWPORT: A 42 px header, two-by-two 228×105 px switch deck, and 44 px expression footer. Position 2 fills its complete tile.
FORM: User-selected Chromatic Deck, first-ranked stage-grid direction, based on mockup 01; fixed physical mapping is the staging rule.
*/
#include "live_renderer.hpp"

#include <algorithm>
#include <array>
#include <string_view>

#include "font_deck.hpp"

namespace midi::display {
namespace {

struct Canvas {
  std::span<std::uint16_t> pixels;
  std::uint16_t width{};
  std::uint16_t rows{};
  std::uint16_t yOffset{};
  std::uint16_t totalHeight{};
};

constexpr char uppercase_ascii(char character) {
  return character >= 'a' && character <= 'z'
             ? static_cast<char>(character - ('a' - 'A'))
             : character;
}

constexpr std::uint16_t blend565(std::uint16_t background,
                                 std::uint16_t foreground,
                                 std::uint8_t alpha) {
  if (alpha == 0) return background;
  if (alpha >= 15) return foreground;
  const auto inverse = static_cast<unsigned>(15 - alpha);
  const auto red = (((background >> 11) & 0x1FU) * inverse +
                    ((foreground >> 11) & 0x1FU) * alpha + 7U) /
                   15U;
  const auto green = (((background >> 5) & 0x3FU) * inverse +
                      ((foreground >> 5) & 0x3FU) * alpha + 7U) /
                     15U;
  const auto blue = ((background & 0x1FU) * inverse +
                     (foreground & 0x1FU) * alpha + 7U) /
                    15U;
  return static_cast<std::uint16_t>((red << 11) | (green << 5) | blue);
}

void put_pixel(Canvas canvas, std::int32_t x, std::int32_t y,
               std::uint16_t color, std::uint8_t alpha = 15) {
  if (x < 0 || y < canvas.yOffset || x >= canvas.width ||
      y >= static_cast<std::int32_t>(canvas.yOffset + canvas.rows) ||
      y >= canvas.totalHeight) {
    return;
  }
  auto& pixel = canvas.pixels[static_cast<std::size_t>(y - canvas.yOffset) *
                                  canvas.width +
                              static_cast<std::size_t>(x)];
  pixel = blend565(pixel, color, alpha);
}

void fill_rect(Canvas canvas, std::int32_t x, std::int32_t y,
               std::int32_t width, std::int32_t height,
               std::uint16_t color) {
  const auto left = std::max<std::int32_t>(0, x);
  const auto right = std::min<std::int32_t>(canvas.width, x + width);
  const auto top = std::max<std::int32_t>(canvas.yOffset, y);
  const auto bottom = std::min<std::int32_t>(canvas.yOffset + canvas.rows,
                                             y + height);
  for (auto row = top; row < bottom; ++row) {
    auto begin = canvas.pixels.begin() +
                 static_cast<std::size_t>(row - canvas.yOffset) * canvas.width +
                 left;
    std::fill(begin, begin + (right - left), color);
  }
}

bool inside_rounded_rect(std::int32_t x, std::int32_t y,
                         std::int32_t width, std::int32_t height,
                         std::int32_t radius) {
  if (x < 0 || y < 0 || x >= width || y >= height) return false;
  if ((x >= radius && x < width - radius) ||
      (y >= radius && y < height - radius)) {
    return true;
  }
  const auto center_x = x < radius ? radius - 1 : width - radius;
  const auto center_y = y < radius ? radius - 1 : height - radius;
  const auto dx = x - center_x;
  const auto dy = y - center_y;
  return dx * dx + dy * dy <= radius * radius;
}

void fill_rounded_rect(Canvas canvas, std::uint16_t radius,
                       std::uint16_t color) {
  for (std::uint16_t row = 0; row < canvas.rows; ++row) {
    const auto y = static_cast<std::int32_t>(canvas.yOffset + row);
    for (std::uint16_t x = 0; x < canvas.width; ++x) {
      if (inside_rounded_rect(x, y, canvas.width, canvas.totalHeight, radius)) {
        canvas.pixels[static_cast<std::size_t>(row) * canvas.width + x] = color;
      }
    }
  }
}

void fill_rounded_rail(Canvas canvas, std::uint16_t rail_width,
                       std::uint16_t radius, std::uint16_t color) {
  for (std::uint16_t row = 0; row < canvas.rows; ++row) {
    const auto y = static_cast<std::int32_t>(canvas.yOffset + row);
    for (std::uint16_t x = 0; x < rail_width; ++x) {
      if (inside_rounded_rect(x, y, canvas.width, canvas.totalHeight, radius)) {
        canvas.pixels[static_cast<std::size_t>(row) * canvas.width + x] = color;
      }
    }
  }
}

void draw_glyph(Canvas canvas, std::int32_t x, std::int32_t y,
                char character, const DeckFont& font,
                std::uint16_t color) {
  const auto normalized = uppercase_ascii(character);
  const auto codepoint = static_cast<unsigned char>(normalized);
  const auto safe_codepoint =
      codepoint >= font.firstCharacter && codepoint <= font.lastCharacter
          ? codepoint
          : static_cast<unsigned char>('?');
  const auto& glyph = font.glyphs[safe_codepoint - font.firstCharacter];
  const auto glyph_x = x + glyph.bearingX;
  const auto glyph_y = y + glyph.bearingY;
  for (std::uint16_t row = 0; row < glyph.height; ++row) {
    for (std::uint16_t column = 0; column < glyph.width; ++column) {
      const auto pixel_index =
          static_cast<std::size_t>(row) * glyph.width + column;
      const auto packed = font.pixels[glyph.offset + pixel_index / 2];
      const auto alpha = static_cast<std::uint8_t>(
          (pixel_index & 1U) == 0 ? packed >> 4 : packed & 0x0FU);
      put_pixel(canvas, glyph_x + column, glyph_y + row, color, alpha);
    }
  }
}

std::int32_t text_width(std::string_view text, const DeckFont& font) {
  std::int32_t width = 0;
  for (const auto character : text) {
    const auto normalized = uppercase_ascii(character);
    const auto codepoint = static_cast<unsigned char>(normalized);
    const auto safe_codepoint =
        codepoint >= font.firstCharacter && codepoint <= font.lastCharacter
            ? codepoint
            : static_cast<unsigned char>('?');
    width += font.glyphs[safe_codepoint - font.firstCharacter].advance;
  }
  return width;
}

std::int32_t draw_text(Canvas canvas, std::int32_t x, std::int32_t y,
                       std::string_view text, DeckFontRole role,
                       std::uint16_t color) {
  const auto& font = deck_font(role);
  auto cursor = x;
  for (const auto character : text) {
    draw_glyph(canvas, cursor, y, character, font, color);
    const auto normalized = uppercase_ascii(character);
    const auto codepoint = static_cast<unsigned char>(normalized);
    const auto safe_codepoint =
        codepoint >= font.firstCharacter && codepoint <= font.lastCharacter
            ? codepoint
            : static_cast<unsigned char>('?');
    cursor += font.glyphs[safe_codepoint - font.firstCharacter].advance;
    if (cursor >= canvas.width) break;
  }
  return cursor;
}

void draw_text_right(Canvas canvas, std::int32_t right, std::int32_t y,
                     std::string_view text, DeckFontRole role,
                     std::uint16_t color) {
  const auto& font = deck_font(role);
  draw_text(canvas, right - text_width(text, font), y, text, role, color);
}

void draw_circle(Canvas canvas, std::int32_t center_x, std::int32_t center_y,
                 std::int32_t radius, std::uint16_t color) {
  for (auto y = center_y - radius; y <= center_y + radius; ++y) {
    for (auto x = center_x - radius; x <= center_x + radius; ++x) {
      const auto dx = x - center_x;
      const auto dy = y - center_y;
      if (dx * dx + dy * dy <= radius * radius) {
        put_pixel(canvas, x, y, color);
      }
    }
  }
}

template <std::size_t Capacity>
std::string_view bounded_text(const AsciiString<Capacity>& value,
                              std::string_view fallback) {
  const auto length = std::min<std::size_t>(value.length, Capacity);
  return length == 0 ? fallback : std::string_view(value.data.data(), length);
}

bool same_ascii(const auto& left, const auto& right) {
  if (left.length != right.length) return false;
  const auto length =
      std::min(left.data.size(), static_cast<std::size_t>(left.length));
  return std::equal(left.data.begin(), left.data.begin() + length,
                    right.data.begin());
}

bool same_position_view(const PositionView& left, const PositionView& right) {
  return same_ascii(left.label, right.label) &&
         left.accentRgb565 == right.accentRgb565;
}

std::string_view decimal_text(std::uint8_t value, std::array<char, 3>& digits,
                              bool pad_two = false) {
  auto count = std::uint8_t{0};
  do {
    digits[2 - count] = static_cast<char>('0' + value % 10U);
    value = static_cast<std::uint8_t>(value / 10U);
    ++count;
  } while (value != 0 && count < digits.size());
  if (pad_two && count == 1) {
    digits[1] = '0';
    count = 2;
  }
  return {digits.data() + digits.size() - count, count};
}

std::uint16_t contrast_text(std::uint16_t color) {
  const auto red = static_cast<unsigned>((color >> 11) & 0x1FU) * 255U / 31U;
  const auto green = static_cast<unsigned>((color >> 5) & 0x3FU) * 255U / 63U;
  const auto blue = static_cast<unsigned>(color & 0x1FU) * 255U / 31U;
  // Squared channels approximate linear-light luminance closely enough for
  // choosing the higher-contrast endpoint on an RGB565 panel. The threshold
  // is the WCAG crossover where black and white have equal contrast (~0.179).
  const auto luminance = 2126U * red * red + 7152U * green * green +
                         722U * blue * blue;
  return luminance > 65025U * 1790U ? ColorBackground : ColorForeground;
}

DeckFontRole title_font(std::string_view text, std::int32_t max_width) {
  for (const auto role : {DeckFontRole::Title, DeckFontRole::CompactTitle,
                          DeckFontRole::Bank}) {
    if (text_width(text, deck_font(role)) <= max_width) return role;
  }
  return DeckFontRole::Bank;
}

}  // namespace

void LiveRenderer::render(const LiveView& view) {
  if (!has_previous_) {
    render_header(view);
    for (unsigned index = 0; index < 4; ++index) render_quadrant(index, view);
    render_footer(view);
  } else {
    if (view.bank != previous_.bank || view.page != previous_.page ||
        !same_ascii(view.bankName, previous_.bankName) ||
        view.usbConnected != previous_.usbConnected ||
        view.configurationError != previous_.configurationError ||
        view.queueOverflow != previous_.queueOverflow ||
        view.watchdogReset != previous_.watchdogReset) {
      render_header(view);
    }
    for (unsigned index = 0; index < 4; ++index) {
      const auto switch_bit = static_cast<std::uint8_t>(1u << index);
      if ((view.pressedMask & switch_bit) !=
              (previous_.pressedMask & switch_bit) ||
          view.positions[index] != previous_.positions[index] ||
          !same_position_view(view.selectedPositions[index],
                              previous_.selectedPositions[index])) {
        render_quadrant(index, view);
      }
    }
    if (view.expressionAvailable != previous_.expressionAvailable ||
        view.expressionValue != previous_.expressionValue ||
        !same_ascii(view.expressionLabel, previous_.expressionLabel)) {
      render_footer(view);
    }
  }
  previous_ = view;
  has_previous_ = true;
}

void LiveRenderer::render_header(const LiveView& view) {
  render_rect({0, 0, ScreenWidth, HeaderHeight}, Region::Header, view, 0,
              ColorPanel);
}

void LiveRenderer::render_quadrant(unsigned index, const LiveView& view) {
  const auto x = static_cast<std::uint16_t>(
      DeckInset + (index % 2) * (QuadrantWidth + DeckGap));
  const auto y = static_cast<std::uint16_t>(
      DeckTop + (index / 2) * (QuadrantHeight + DeckGap));
  const auto configured = view.selectedPositions[index].accentRgb565;
  const auto accent = configured == 0 ? ColorAccent : configured;
  render_rect({x, y, QuadrantWidth, QuadrantHeight}, Region::Quadrant, view,
              index, accent);
}

void LiveRenderer::render_footer(const LiveView& view) {
  render_rect({0, FooterY, ScreenWidth, FooterHeight}, Region::Footer, view, 0,
              ColorPanel);
}

void LiveRenderer::render_rect(Rect rect, Region region, const LiveView& view,
                               unsigned quadrant_index,
                               std::uint16_t accent) {
  if (rect.width == 0 || rect.height == 0 || rect.width > ScreenWidth) return;
  const auto rows_per_transfer =
      static_cast<std::uint16_t>(TilePixels / rect.width);
  const auto transfer_rows = std::max<std::uint16_t>(1, rows_per_transfer);
  for (std::uint16_t y_offset = 0; y_offset < rect.height;
       y_offset += transfer_rows) {
    const auto rows =
        std::min<std::uint16_t>(transfer_rows, rect.height - y_offset);
    const auto pixel_count = static_cast<std::size_t>(rect.width) * rows;
    auto pixels = std::span<std::uint16_t>(pixels_.data(), pixel_count);
    std::fill(pixels.begin(), pixels.end(), ColorBackground);
    const Canvas canvas{pixels, rect.width, rows, y_offset, rect.height};

    if (region == Region::Header) {
      fill_rect(canvas, 0, 0, rect.width, rect.height, ColorPanel);
      std::array<char, 3> bank_digits{};
      draw_text(canvas, 16, 1, "BANK", DeckFontRole::Label, ColorMuted);
      draw_text(canvas, 55, 1, decimal_text(view.bank, bank_digits, true),
                DeckFontRole::Label, ColorMuted);
      draw_text(canvas, 16, 12, bounded_text(view.bankName, "FACTORY"),
                DeckFontRole::Bank, ColorForeground);

      if (view.configurationError) {
        draw_text_right(canvas, 463, 10, "CONFIG!", DeckFontRole::Label,
                        ColorError);
      } else if (view.queueOverflow) {
        draw_text_right(canvas, 463, 10, "QUEUE!", DeckFontRole::Label,
                        ColorWarning);
      } else if (view.watchdogReset) {
        draw_text_right(canvas, 463, 10, "WATCHDOG!", DeckFontRole::Label,
                        ColorWarning);
      } else {
        const std::array<char, 7> page_text{
            'P', ' ', static_cast<char>('0' + view.page), ' ', '/', ' ', '4'};
        draw_text(canvas, 355, 10,
                  std::string_view(page_text.data(), page_text.size()),
                  DeckFontRole::Label, ColorMuted);
        draw_text(canvas, 417, 10, view.usbConnected ? "USB" : "USB-",
                  DeckFontRole::Label,
                  view.usbConnected ? ColorForeground : ColorMuted);
        draw_circle(canvas, 461, 21, 5,
                    view.usbConnected ? ColorSuccess : ColorMuted);
      }
    } else if (region == Region::Quadrant) {
      const auto& position = view.selectedPositions[quadrant_index];
      const auto active = view.positions[quadrant_index] == 2;
      const auto pressed =
          (view.pressedMask & static_cast<std::uint8_t>(1u << quadrant_index)) !=
          0;
      const auto filled = active || pressed;
      const auto foreground = filled ? contrast_text(accent) : ColorForeground;
      fill_rounded_rect(canvas, 10, filled ? accent : ColorTile);
      if (!filled) fill_rounded_rail(canvas, 6, 10, accent);

      const std::array<char, 1> switch_name{
          static_cast<char>('A' + quadrant_index)};
      draw_text(canvas, 18, -5,
                std::string_view(switch_name.data(), switch_name.size()),
                DeckFontRole::Switch,
                filled ? foreground : accent);
      const auto label = bounded_text(position.label, "EMPTY");
      const auto role = title_font(label, rect.width - 78);
      draw_text(canvas, 68, 15, label, role, foreground);
      draw_text(canvas, 68, 60,
                pressed ? (active ? std::string_view("PRESSED / P2")
                                  : std::string_view("PRESSED / P1"))
                        : (active ? std::string_view("POSITION 2 / ON")
                                  : std::string_view("POSITION 1")),
                DeckFontRole::Label,
                pressed ? foreground : (active ? foreground : ColorMuted));
      fill_rect(canvas, 68, 84, filled ? 128 : 88, 3,
                filled ? foreground : accent);
    } else {
      fill_rect(canvas, 0, 0, rect.width, rect.height, ColorPanel);
      auto cursor = draw_text(canvas, 16, 10, "EXP / ", DeckFontRole::Label,
                              ColorForeground);
      draw_text(canvas, cursor, 10,
                bounded_text(view.expressionLabel, "NONE"),
                DeckFontRole::Label,
                view.expressionAvailable ? ColorForeground : ColorMuted);
      fill_rect(canvas, 175, 17, 237, 6, ColorMeterTrack);
      if (view.expressionAvailable) {
        const auto meter_width = static_cast<std::int32_t>(
            static_cast<unsigned>(view.expressionValue) * 237U / 127U);
        fill_rect(canvas, 175, 17, meter_width, 6, ColorAccent);
        std::array<char, 3> value_digits{};
        const auto value = decimal_text(view.expressionValue, value_digits);
        draw_text_right(canvas, 463, 4, value, DeckFontRole::CompactTitle,
                        ColorAccent);
      } else {
        draw_text_right(canvas, 463, 4, "--", DeckFontRole::CompactTitle,
                        ColorMuted);
      }
    }

    target_.write(
        {rect.x, static_cast<std::uint16_t>(rect.y + y_offset), rect.width,
         rows},
        pixels);
  }
}

}  // namespace midi::display
