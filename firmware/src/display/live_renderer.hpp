#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "../hal/ports.hpp"

namespace midi::display {

inline constexpr std::uint16_t ScreenWidth = 480;
inline constexpr std::uint16_t ScreenHeight = 320;
inline constexpr std::uint16_t HeaderHeight = 48;
inline constexpr std::uint16_t FooterHeight = 48;
inline constexpr std::uint16_t FooterY = ScreenHeight - FooterHeight;
inline constexpr std::uint16_t QuadrantWidth = ScreenWidth / 2;
inline constexpr std::uint16_t QuadrantHeight = (ScreenHeight - HeaderHeight - FooterHeight) / 2;

inline constexpr std::uint16_t ColorBackground = 0x0000;
inline constexpr std::uint16_t ColorPanel = 0x18E3;
inline constexpr std::uint16_t ColorForeground = 0xFFFF;
inline constexpr std::uint16_t ColorMuted = 0x8410;
inline constexpr std::uint16_t ColorAccent = 0x07FF;
inline constexpr std::uint16_t ColorSuccess = 0x07E0;
inline constexpr std::uint16_t ColorError = 0xF800;
inline constexpr std::uint16_t ColorWarning = 0xFD20;

struct Rect {
  std::uint16_t x{};
  std::uint16_t y{};
  std::uint16_t width{};
  std::uint16_t height{};
};

class RenderTarget {
 public:
  virtual ~RenderTarget() = default;
  virtual void write(Rect rect, std::span<const std::uint16_t> pixels) = 0;
};

class LiveRenderer {
 public:
  explicit LiveRenderer(RenderTarget& target) : target_(target) {}

  void render(const LiveView& view);
  void invalidate() { has_previous_ = false; }

 private:
  static constexpr std::size_t TilePixels = 2048;
  enum class Region : std::uint8_t { Header, Quadrant, Footer };

  void render_header(const LiveView& view);
  void render_quadrant(unsigned index, const LiveView& view);
  void render_footer(const LiveView& view);
  void render_rect(Rect rect, Region region, const LiveView& view,
                   unsigned quadrant_index, std::uint16_t border);

  RenderTarget& target_;
  LiveView previous_{};
  bool has_previous_{};
  std::array<std::uint16_t, TilePixels> pixels_{};
};

}  // namespace midi::display
