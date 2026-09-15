#pragma once

#include <cstdint>
#include <span>

#include "../hal/ports.hpp"
#include "live_renderer.hpp"

namespace midi::display {

class St7796sDisplay final : public DisplayPort, public RenderTarget {
 public:
  static constexpr std::uint32_t SpiFrequencyHz = 8'000'000;
  static constexpr std::uint8_t PixelFormatRgb565 = 0x55;
  static constexpr std::uint8_t MadctlLandscape = 0x28;

  void initialize();
  void present(const LiveView& view) override;
  void write(Rect rect, std::span<const std::uint16_t> pixels) override;

 private:
  void command(std::uint8_t value,
               std::span<const std::uint8_t> parameters = {});
  void set_window(Rect rect);
  void flush_frame();

  LiveRenderer renderer_{*this};
  bool initialized_{};
  bool frame_dirty_{};
};

}  // namespace midi::display
