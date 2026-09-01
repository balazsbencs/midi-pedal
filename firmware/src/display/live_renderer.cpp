#include "live_renderer.hpp"

#include <algorithm>
#include <array>
#include <string_view>

#include "font_ascii.hpp"

namespace midi::display {
namespace {

constexpr std::uint16_t clamp_width(std::uint16_t width) {
  return width > ScreenWidth ? ScreenWidth : width;
}

void fill(std::span<std::uint16_t> pixels, std::uint16_t color) {
  std::fill(pixels.begin(), pixels.end(), color);
}

void draw_border(std::span<std::uint16_t> pixels, std::uint16_t width,
                 std::uint16_t height, std::uint16_t color) {
  if (width == 0 || height == 0) return;
  for (std::uint16_t x = 0; x < width; ++x) {
    pixels[x] = color;
    pixels[static_cast<std::size_t>(height - 1) * width + x] = color;
  }
  for (std::uint16_t y = 0; y < height; ++y) {
    pixels[static_cast<std::size_t>(y) * width] = color;
    pixels[static_cast<std::size_t>(y) * width + width - 1] = color;
  }
}

void draw_glyph(std::span<std::uint16_t> pixels, std::uint16_t width,
                std::uint16_t height, std::int32_t x, std::int32_t y,
                char character, std::uint16_t color, std::uint8_t scale = 2) {
  const auto glyph = glyph_for(character);
  for (std::uint16_t row = 0; row < glyph.size(); ++row) {
    for (std::uint16_t col = 0; col < 5; ++col) {
      if ((glyph[row] & (1u << (4u - col))) == 0) continue;
      for (std::uint8_t sy = 0; sy < scale; ++sy) {
        for (std::uint8_t sx = 0; sx < scale; ++sx) {
          const auto px = x + static_cast<std::int32_t>(col * scale + sx);
          const auto py = y + static_cast<std::int32_t>(row * scale + sy);
          if (px >= 0 && py >= 0 && px < width && py < height) {
            pixels[static_cast<std::size_t>(py) * width + static_cast<std::size_t>(px)] = color;
          }
        }
      }
    }
  }
}

void draw_text(std::span<std::uint16_t> pixels, std::uint16_t width,
               std::uint16_t height, std::int32_t x, std::int32_t y,
               std::string_view text, std::uint16_t color,
               std::uint8_t scale = 2) {
  auto cursor = x;
  for (const char character : text) {
    draw_glyph(pixels, width, height, cursor, y, character, color, scale);
    cursor += 6 * scale;
    if (cursor >= width) break;
  }
}

template <std::size_t Capacity>
std::string_view bounded_text(const AsciiString<Capacity>& value, std::string_view fallback) {
  const auto length = std::min<std::size_t>(value.length, Capacity);
  return length == 0 ? fallback : std::string_view(value.data.data(), length);
}

bool same_ascii(const auto& left, const auto& right) {
  if (left.length != right.length) return false;
  const auto length = std::min(left.data.size(), static_cast<std::size_t>(left.length));
  return std::equal(left.data.begin(), left.data.begin() + length, right.data.begin());
}

bool same_position_view(const PositionView& left, const PositionView& right) {
  return same_ascii(left.label, right.label) && left.accentRgb565 == right.accentRgb565;
}

void draw_decimal(std::span<std::uint16_t> pixels, std::uint16_t width, std::uint16_t height,
                  std::int32_t x, std::int32_t y, std::uint8_t value, std::uint16_t color) {
  std::array<char, 3> digits{};
  std::uint8_t count = 0;
  do {
    digits[2 - count] = static_cast<char>('0' + value % 10u);
    value = static_cast<std::uint8_t>(value / 10u);
    ++count;
  } while (value != 0);
  for (std::uint8_t index = 0; index < count; ++index) {
    draw_glyph(pixels, width, height, x + index * 12, y, digits[3 - count + index], color, 2);
  }
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
      if (view.positions[index] != previous_.positions[index] ||
          !same_position_view(view.selectedPositions[index], previous_.selectedPositions[index])) {
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
  render_rect({0, 0, ScreenWidth, HeaderHeight}, Region::Header, view, 0, ColorPanel);
}

void LiveRenderer::render_quadrant(unsigned index, const LiveView& view) {
  const auto x = static_cast<std::uint16_t>((index % 2) * QuadrantWidth);
  const auto y = static_cast<std::uint16_t>(HeaderHeight + (index / 2) * QuadrantHeight);
  const auto configured = view.selectedPositions[index].accentRgb565;
  const auto accent = configured == 0 ? ColorAccent : configured;
  render_rect({x, y, QuadrantWidth, QuadrantHeight}, Region::Quadrant, view, index, accent);
}

void LiveRenderer::render_footer(const LiveView& view) {
  render_rect({0, FooterY, ScreenWidth, FooterHeight}, Region::Footer, view, 0, ColorPanel);
}

void LiveRenderer::render_rect(Rect rect, Region region, const LiveView& view,
                               unsigned quadrant_index, std::uint16_t border) {
  rect.width = clamp_width(rect.width);
  if (rect.width == 0 || rect.height == 0) return;
  const auto rows_per_transfer = static_cast<std::uint16_t>(TilePixels / rect.width);
  const auto transfer_rows = std::max<std::uint16_t>(1, rows_per_transfer);
  for (std::uint16_t y_offset = 0; y_offset < rect.height; y_offset += transfer_rows) {
    const auto rows = std::min<std::uint16_t>(transfer_rows, rect.height - y_offset);
    const auto pixel_count = static_cast<std::size_t>(rect.width) * rows;
    const auto pixels = std::span<std::uint16_t>(pixels_.data(), pixel_count);
    fill(pixels, ColorBackground);
    draw_border(pixels, rect.width, rows, border);
    const auto origin_y = static_cast<std::int32_t>(rect.y + y_offset);

    if (region == Region::Header) {
      const auto status_color = view.configurationError ? ColorError : ColorForeground;
      draw_text(pixels, rect.width, rows, 12, 7 - origin_y,
                bounded_text(view.bankName, "EMPTY"), status_color, 2);
      draw_text(pixels, rect.width, rows, 264, 7 - origin_y, "B", ColorMuted, 2);
      draw_decimal(pixels, rect.width, rows, 276, 7 - origin_y, view.bank, status_color);
      draw_text(pixels, rect.width, rows, 316, 7 - origin_y, "P", ColorMuted, 2);
      draw_decimal(pixels, rect.width, rows, 328, 7 - origin_y, view.page, status_color);
      draw_text(pixels, rect.width, rows, 346, 7 - origin_y, view.usbConnected ? "USB+" : "USB-",
                view.usbConnected ? ColorSuccess : ColorMuted, 2);
      draw_text(pixels, rect.width, rows, 398, 7 - origin_y, view.configurationError ? "C!" : "C+",
                view.configurationError ? ColorError : ColorSuccess, 2);
      if (view.queueOverflow) draw_text(pixels, rect.width, rows, 424, 7 - origin_y, "Q!", ColorWarning, 2);
      if (view.watchdogReset) draw_text(pixels, rect.width, rows, 450, 7 - origin_y, "W!", ColorWarning, 2);
    } else if (region == Region::Quadrant) {
      const auto& position = view.selectedPositions[quadrant_index];
      const auto accent = position.accentRgb565 == 0 ? ColorAccent : position.accentRgb565;
      draw_glyph(pixels, rect.width, rows, 16, static_cast<std::int32_t>(rect.y + 18) - origin_y,
                 static_cast<char>('A' + quadrant_index), ColorForeground, 3);
      draw_text(pixels, rect.width, rows, 64, static_cast<std::int32_t>(rect.y + 18) - origin_y,
                bounded_text(position.label, "EMPTY"), accent, 2);
      draw_text(pixels, rect.width, rows, 64, static_cast<std::int32_t>(rect.y + 66) - origin_y,
                view.positions[quadrant_index] == 2 ? "P2" : "P1", accent, 2);
    } else {
      const auto label = bounded_text(view.expressionLabel, "NONE");
      draw_text(pixels, rect.width, rows, 12, static_cast<std::int32_t>(FooterY + 8) - origin_y,
                "EXPR", ColorForeground, 2);
      draw_text(pixels, rect.width, rows, 72, static_cast<std::int32_t>(FooterY + 8) - origin_y,
                label, view.expressionAvailable ? ColorForeground : ColorMuted, 2);
      if (!view.expressionAvailable) {
        draw_text(pixels, rect.width, rows, 408, static_cast<std::int32_t>(FooterY + 8) - origin_y,
                  "-", ColorMuted, 2);
      } else {
        draw_decimal(pixels, rect.width, rows, 432, static_cast<std::int32_t>(FooterY + 8) - origin_y,
                     view.expressionValue, ColorForeground);
        const auto bar_width = static_cast<std::uint16_t>((static_cast<unsigned>(view.expressionValue) * 160u) / 127u);
        for (std::uint16_t x = 0; x < bar_width; ++x) {
          for (std::uint16_t y = 30; y < 38; ++y) {
            const auto local_y = static_cast<std::int32_t>(FooterY + y) - origin_y;
            if (local_y >= 0 && local_y < rows && x + 240 < rect.width) {
              pixels[static_cast<std::size_t>(local_y) * rect.width + 240 + x] = ColorAccent;
            }
          }
        }
      }
    }
    target_.write({rect.x, static_cast<std::uint16_t>(rect.y + y_offset), rect.width, rows}, pixels);
  }
}

}  // namespace midi::display
