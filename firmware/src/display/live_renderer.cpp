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

}  // namespace

void LiveRenderer::render(const LiveView& view) {
  if (!has_previous_) {
    render_header(view);
    for (unsigned index = 0; index < 4; ++index) render_quadrant(index, view.positions[index]);
    render_footer(view);
  } else {
    if (view.bank != previous_.bank || view.page != previous_.page ||
        view.usbConnected != previous_.usbConnected ||
        view.configurationError != previous_.configurationError ||
        view.queueOverflow != previous_.queueOverflow) {
      render_header(view);
    }
    for (unsigned index = 0; index < 4; ++index) {
      if (view.positions[index] != previous_.positions[index]) render_quadrant(index, view.positions[index]);
    }
    if (view.expressionAvailable != previous_.expressionAvailable ||
        view.expressionValue != previous_.expressionValue) render_footer(view);
  }
  previous_ = view;
  has_previous_ = true;
}

void LiveRenderer::render_header(const LiveView& view) {
  render_rect(Rect{0, 0, ScreenWidth, HeaderHeight}, ColorBackground, ColorPanel,
              static_cast<std::uint8_t>('M'), view.bank, false,
              view.configurationError, view.queueOverflow);
}

void LiveRenderer::render_quadrant(unsigned index, std::uint8_t position) {
  const auto x = static_cast<std::uint16_t>((index % 2) * QuadrantWidth);
  const auto y = static_cast<std::uint16_t>(HeaderHeight + (index / 2) * QuadrantHeight);
  render_rect(Rect{x, y, QuadrantWidth, QuadrantHeight}, ColorBackground, ColorPanel,
              static_cast<std::uint8_t>('A' + index), position, false, false, false);
}

void LiveRenderer::render_footer(const LiveView& view) {
  render_rect(Rect{0, FooterY, ScreenWidth, FooterHeight}, ColorBackground, ColorPanel,
              static_cast<std::uint8_t>('E'), view.expressionValue,
              !view.expressionAvailable, false, false);
}

void LiveRenderer::render_rect(Rect rect, std::uint16_t fill_color, std::uint16_t border,
                               std::uint8_t text_code, std::uint8_t value,
                               bool unavailable, bool error, bool warning) {
  rect.width = clamp_width(rect.width);
  if (rect.width == 0 || rect.height == 0) return;
  const auto rows_per_transfer = static_cast<std::uint16_t>(TilePixels / rect.width);
  const auto transfer_rows = std::max<std::uint16_t>(1, rows_per_transfer);
  for (std::uint16_t y_offset = 0; y_offset < rect.height; y_offset += transfer_rows) {
    const auto rows = std::min<std::uint16_t>(transfer_rows, rect.height - y_offset);
    const auto pixel_count = static_cast<std::size_t>(rect.width) * rows;
    fill(std::span<std::uint16_t>(pixels_.data(), pixel_count), fill_color);
    draw_border(std::span<std::uint16_t>(pixels_.data(), pixel_count), rect.width, rows, border);
    const auto text_color = error ? ColorError : warning ? ColorWarning : ColorForeground;
    const auto origin_y = static_cast<std::int32_t>(rect.y + y_offset);
    if (rect.y == 0) {
      draw_text(std::span<std::uint16_t>(pixels_.data(), pixel_count), rect.width, rows,
                12, 12 - origin_y, "MIDI", text_color, 2);
      draw_text(std::span<std::uint16_t>(pixels_.data(), pixel_count), rect.width, rows,
                180, 12 - origin_y, "B", ColorMuted, 2);
      draw_glyph(std::span<std::uint16_t>(pixels_.data(), pixel_count), rect.width, rows,
                 198, 12 - origin_y, static_cast<char>('0' + (value / 10) % 10), text_color, 2);
      draw_glyph(std::span<std::uint16_t>(pixels_.data(), pixel_count), rect.width, rows,
                 210, 12 - origin_y, static_cast<char>('0' + value % 10), text_color, 2);
      draw_text(std::span<std::uint16_t>(pixels_.data(), pixel_count), rect.width, rows,
                276, 12 - origin_y, "USB", ColorMuted, 2);
    } else if (rect.y >= FooterY) {
      draw_text(std::span<std::uint16_t>(pixels_.data(), pixel_count), rect.width, rows,
                12, static_cast<std::int32_t>(FooterY + 12) - origin_y,
                unavailable ? "EXPR -" : "EXPR", text_color, 2);
      if (!unavailable) {
        const auto bar_width = static_cast<std::uint16_t>((static_cast<unsigned>(value) * 300u) / 127u);
        for (std::uint16_t x = 0; x < bar_width && x + 120 < rect.width; ++x) {
          for (std::uint16_t yy = 14; yy < 22; ++yy) {
            const auto local_y = static_cast<std::int32_t>(FooterY + yy) - origin_y;
            if (local_y >= 0 && local_y < rows) pixels_[local_y * rect.width + 120 + x] = ColorAccent;
          }
        }
      }
    } else {
      const auto label_x = static_cast<std::uint16_t>(rect.width / 2 - 24);
      draw_glyph(std::span<std::uint16_t>(pixels_.data(), pixel_count), rect.width, rows,
                 label_x, static_cast<std::int32_t>(rect.y + 28) - origin_y,
                 static_cast<char>(text_code), ColorForeground, 4);
      draw_text(std::span<std::uint16_t>(pixels_.data(), pixel_count), rect.width, rows,
                static_cast<std::int32_t>(rect.width / 2 - 12),
                static_cast<std::int32_t>(rect.y + 116) - origin_y,
                value == 2 ? "P2" : "P1", text_color, 2);
    }
    const Rect transfer_rect{rect.x, static_cast<std::uint16_t>(rect.y + y_offset), rect.width, rows};
    target_.write(transfer_rect, std::span<const std::uint16_t>(pixels_.data(), pixel_count));
  }
}

}  // namespace midi::display
