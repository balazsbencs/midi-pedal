#include "st7796s.hpp"

#include <algorithm>
#include <array>

#include "../board/pico2_pins.hpp"

#ifdef PICO_ON_DEVICE
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#endif

namespace midi::display {
namespace {

#ifdef PICO_ON_DEVICE
constexpr std::size_t ScanlineBytes = ScreenWidth * 2;
alignas(4) std::array<std::uint8_t,
                      static_cast<std::size_t>(ScreenWidth) * ScreenHeight * 2>
    frame_bytes{};
#endif

}  // namespace

void St7796sDisplay::initialize() {
#ifdef PICO_ON_DEVICE
  gpio_init(board::DisplayCs);
  gpio_init(board::DisplayDc);
  gpio_init(board::DisplayReset);
  gpio_put(board::DisplayCs, 1);
  gpio_put(board::DisplayDc, 1);
  gpio_put(board::DisplayReset, 1);
  gpio_set_dir(board::DisplayCs, GPIO_OUT);
  gpio_set_dir(board::DisplayDc, GPIO_OUT);
  gpio_set_dir(board::DisplayReset, GPIO_OUT);

  spi_init(spi0, SpiFrequencyHz);
  spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_set_function(board::DisplaySck, GPIO_FUNC_SPI);
  gpio_set_function(board::DisplayMosi, GPIO_FUNC_SPI);

  sleep_ms(20);
  gpio_put(board::DisplayReset, 0);
  sleep_ms(120);
  gpio_put(board::DisplayReset, 1);
  sleep_ms(200);

  command(0x01);  // Software reset.
  sleep_ms(150);

  // ST7796S manufacturer sequence used by the known-good display test.
  command(0xF0, std::array<std::uint8_t, 1>{0xC3});
  command(0xF0, std::array<std::uint8_t, 1>{0x96});
  command(0xC5, std::array<std::uint8_t, 1>{0x1C});
  command(0x36, std::array<std::uint8_t, 1>{MadctlLandscape});
  command(0x3A, std::array<std::uint8_t, 1>{PixelFormatRgb565});
  command(0xB0, std::array<std::uint8_t, 1>{0x80});
  command(0xB4, std::array<std::uint8_t, 1>{0x00});
  command(0xB6, std::array<std::uint8_t, 3>{0x80, 0x02, 0x3B});
  command(0xB7, std::array<std::uint8_t, 1>{0xC6});
  command(0xF0, std::array<std::uint8_t, 1>{0x69});
  command(0xF0, std::array<std::uint8_t, 1>{0x3C});

  command(0x11);  // Sleep out.
  sleep_ms(150);
  command(0x13);  // Normal display mode.
  command(0x20);  // Display inversion off; this panel renders logical RGB565 directly.
  command(0x29);  // Display on.
  sleep_ms(150);
  initialized_ = true;
#else
  initialized_ = true;
#endif
}

void St7796sDisplay::command(
    std::uint8_t value, std::span<const std::uint8_t> parameters) {
#ifdef PICO_ON_DEVICE
  gpio_put(board::DisplayCs, 0);
  gpio_put(board::DisplayDc, 0);
  spi_write_blocking(spi0, &value, 1);
  if (!parameters.empty()) {
    gpio_put(board::DisplayDc, 1);
    spi_write_blocking(spi0, parameters.data(), parameters.size());
  }
  gpio_put(board::DisplayCs, 1);
#else
  (void)value;
  (void)parameters;
#endif
}

void St7796sDisplay::set_window(Rect rect) {
#ifdef PICO_ON_DEVICE
  const auto x1 = static_cast<std::uint16_t>(rect.x + rect.width - 1);
  const auto y1 = static_cast<std::uint16_t>(rect.y + rect.height - 1);
  const std::array<std::uint8_t, 4> columns{
      static_cast<std::uint8_t>(rect.x >> 8), static_cast<std::uint8_t>(rect.x),
      static_cast<std::uint8_t>(x1 >> 8), static_cast<std::uint8_t>(x1)};
  const std::array<std::uint8_t, 4> rows{
      static_cast<std::uint8_t>(rect.y >> 8), static_cast<std::uint8_t>(rect.y),
      static_cast<std::uint8_t>(y1 >> 8), static_cast<std::uint8_t>(y1)};
  command(0x2A, columns);
  command(0x2B, rows);
#else
  (void)rect;
#endif
}

void St7796sDisplay::present(const LiveView& view) {
  if (!initialized_) return;
  renderer_.render(view);
}

void St7796sDisplay::write(Rect rect, std::span<const std::uint16_t> pixels) {
  if (!initialized_ || rect.width == 0 || rect.height == 0) return;
  if (rect.x >= ScreenWidth || rect.y >= ScreenHeight ||
      rect.width > ScreenWidth - rect.x ||
      rect.height > ScreenHeight - rect.y) {
    return;
  }
  const auto expected = static_cast<std::size_t>(rect.width) * rect.height;
  if (pixels.size() < expected) return;
#ifdef PICO_ON_DEVICE
  for (std::uint16_t row = 0; row < rect.height; ++row) {
    for (std::uint16_t column = 0; column < rect.width; ++column) {
      const auto source = static_cast<std::size_t>(row) * rect.width + column;
      const auto destination =
          (static_cast<std::size_t>(rect.y + row) * ScreenWidth + rect.x +
           column) *
          2;
      frame_bytes[destination] =
          static_cast<std::uint8_t>(pixels[source] >> 8);
      frame_bytes[destination + 1] =
          static_cast<std::uint8_t>(pixels[source]);
    }
  }
  mark_dirty(rect);
#else
  (void)pixels;
#endif
}

void St7796sDisplay::mark_dirty(Rect rect) {
  (void)rect;
  // This module handles a full 480x320 window reliably, but ignores or
  // misapplies partial vertical windows. Keep the proven full-screen address
  // window while service() spreads its transfer over short control-loop steps.
  pending_rect_ = {0, 0, ScreenWidth, ScreenHeight};
  dirty_pending_ = true;
}

void St7796sDisplay::begin_transfer() {
#ifdef PICO_ON_DEVICE
  if (!dirty_pending_ || transfer_active_) return;
  transfer_rect_ = pending_rect_;
  dirty_pending_ = false;
  transfer_row_ = 0;
  set_window(transfer_rect_);
  const std::uint8_t memory_write = 0x2C;
  gpio_put(board::DisplayCs, 0);
  gpio_put(board::DisplayDc, 0);
  spi_write_blocking(spi0, &memory_write, 1);
  gpio_put(board::DisplayDc, 1);
  transfer_active_ = true;
#endif
}

void St7796sDisplay::service() {
#ifdef PICO_ON_DEVICE
  if (!initialized_) return;
  if (!transfer_active_) begin_transfer();
  if (!transfer_active_) return;

  const auto rows = std::min<std::uint16_t>(
      RowsPerService, transfer_rect_.height - transfer_row_);
  const auto bytes_per_row = static_cast<std::size_t>(transfer_rect_.width) * 2;
  for (std::uint16_t index = 0; index < rows; ++index) {
    const auto row = static_cast<std::uint16_t>(transfer_rect_.y +
                                                transfer_row_ + index);
    const auto offset = static_cast<std::size_t>(row) * ScanlineBytes +
                        static_cast<std::size_t>(transfer_rect_.x) * 2;
    spi_write_blocking(spi0, frame_bytes.data() + offset, bytes_per_row);
  }
  transfer_row_ = static_cast<std::uint16_t>(transfer_row_ + rows);
  if (transfer_row_ == transfer_rect_.height) {
    gpio_put(board::DisplayCs, 1);
    transfer_active_ = false;
  }
#endif
}

}  // namespace midi::display
