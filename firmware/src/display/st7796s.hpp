#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "../hal/ports.hpp"
#include "live_renderer.hpp"

namespace midi::display {

class St7796sDisplay final : public DisplayPort, public RenderTarget {
 public:
  static constexpr std::uint32_t SpiFrequencyHz = 32'000'000;
  static constexpr std::uint8_t PixelFormatRgb565 = 0x55;
  static constexpr std::uint8_t MadctlLandscape = 0x28;

  void initialize();
  void present(const LiveView& view) override { renderer_.render(view); }
  void write(Rect rect, std::span<const std::uint16_t> pixels) override;

 private:
  void command(std::uint8_t value);
  void data(std::span<const std::uint8_t> bytes);
  void set_window(Rect rect);
  void wait_for_transfer();

  LiveRenderer renderer_{*this};
  std::array<std::uint8_t, 4096> transfer_bytes_{};
  int dma_channel_{-1};
  bool initialized_{};
};

}  // namespace midi::display

