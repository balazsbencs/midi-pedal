#pragma once

#include <array>
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
  // Queues a bounded DMA chunk; returns immediately while the wire is busy.
  // Call once per control-loop iteration after time-critical I/O has run.
  void service();

 private:
  static constexpr std::uint16_t RowsPerService = 4;

  void command(std::uint8_t value,
               std::span<const std::uint8_t> parameters = {});
  void set_window(Rect rect);
  void mark_dirty(Rect rect);
  void begin_transfer();

  LiveRenderer renderer_{*this};
  std::array<std::uint8_t, ScreenWidth * RowsPerService * 2> transfer_bytes_{};
  int dma_channel_{-1};
  bool chunk_pending_{};
  std::uint16_t chunk_rows_{};
  bool initialized_{};
  bool dirty_pending_{};
  bool transfer_active_{};
  Rect pending_rect_{};
  Rect transfer_rect_{};
  std::uint16_t transfer_row_{};
};

}  // namespace midi::display
