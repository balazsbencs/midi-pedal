#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "../hal/ports.hpp"

namespace midi::display {

inline constexpr std::uint16_t ScreenWidth = 480;
inline constexpr std::uint16_t ScreenHeight = 320;
inline constexpr std::uint16_t HeaderHeight = 42;
inline constexpr std::uint16_t FooterHeight = 44;
inline constexpr std::uint16_t FooterY = ScreenHeight - FooterHeight;
inline constexpr std::uint16_t DeckInset = 8;
inline constexpr std::uint16_t DeckGap = 8;
inline constexpr std::uint16_t DeckTop = HeaderHeight + DeckGap;
inline constexpr std::uint16_t QuadrantWidth = 228;
inline constexpr std::uint16_t QuadrantHeight = 105;

inline constexpr std::uint16_t ColorBackground = 0x0082;
inline constexpr std::uint16_t ColorPanel = 0x08C3;
inline constexpr std::uint16_t ColorTile = 0x1105;
inline constexpr std::uint16_t ColorForeground = 0xF7DF;
inline constexpr std::uint16_t ColorMuted = 0x6C11;
inline constexpr std::uint16_t ColorAccent = 0x2F3A;
inline constexpr std::uint16_t ColorSuccess = 0x4FB4;
inline constexpr std::uint16_t ColorError = 0xF2EB;
inline constexpr std::uint16_t ColorWarning = 0xFE48;
inline constexpr std::uint16_t ColorMeterTrack = 0x21C7;

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
