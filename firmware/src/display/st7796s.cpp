#include "st7796s.hpp"

#include <algorithm>

#include "../board/pico2_pins.hpp"

#ifdef PICO_ON_DEVICE
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#endif

namespace midi::display {

void St7796sDisplay::initialize() {
#ifdef PICO_ON_DEVICE
  spi_init(spi0, SpiFrequencyHz);
  gpio_set_function(board::DisplaySck, GPIO_FUNC_SPI);
  gpio_set_function(board::DisplayMosi, GPIO_FUNC_SPI);
  gpio_init(board::DisplayCs);
  gpio_init(board::DisplayDc);
  gpio_init(board::DisplayReset);
  gpio_set_dir(board::DisplayCs, GPIO_OUT);
  gpio_set_dir(board::DisplayDc, GPIO_OUT);
  gpio_set_dir(board::DisplayReset, GPIO_OUT);
  gpio_put(board::DisplayCs, 1);
  gpio_put(board::DisplayDc, 1);
  gpio_put(board::DisplayReset, 0);
  sleep_ms(10);
  gpio_put(board::DisplayReset, 1);
  sleep_ms(120);

  dma_channel_ = dma_claim_unused_channel(true);
  command(0x01);  // software reset
  sleep_ms(5);
  command(0x11);  // sleep out
  sleep_ms(120);
  command(0x3A);
  const std::uint8_t format = PixelFormatRgb565;
  data(std::span<const std::uint8_t>(&format, 1));
  command(0x36);
  const std::uint8_t orientation = MadctlLandscape;
  data(std::span<const std::uint8_t>(&orientation, 1));
  command(0x29);  // display on
  sleep_ms(20);
  initialized_ = true;
#else
  initialized_ = true;
#endif
}

void St7796sDisplay::command(std::uint8_t value) {
#ifdef PICO_ON_DEVICE
  gpio_put(board::DisplayDc, 0);
  gpio_put(board::DisplayCs, 0);
  spi_write_blocking(spi0, &value, 1);
  gpio_put(board::DisplayCs, 1);
#else
  (void)value;
#endif
}

void St7796sDisplay::data(std::span<const std::uint8_t> bytes) {
#ifdef PICO_ON_DEVICE
  if (bytes.empty()) return;
  gpio_put(board::DisplayDc, 1);
  gpio_put(board::DisplayCs, 0);
  spi_write_blocking(spi0, bytes.data(), bytes.size());
  gpio_put(board::DisplayCs, 1);
#else
  (void)bytes;
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
  command(0x2A);
  data(columns);
  command(0x2B);
  data(rows);
  command(0x2C);
#else
  (void)rect;
#endif
}

void St7796sDisplay::wait_for_transfer() {
#ifdef PICO_ON_DEVICE
  if (dma_channel_ >= 0) dma_channel_wait_for_finish_blocking(dma_channel_);
#endif
}

void St7796sDisplay::write(Rect rect, std::span<const std::uint16_t> pixels) {
  if (!initialized_ || rect.width == 0 || rect.height == 0) return;
  const auto expected = static_cast<std::size_t>(rect.width) * rect.height;
  const auto count = std::min(expected, pixels.size());
#ifdef PICO_ON_DEVICE
  wait_for_transfer();
  set_window(rect);
  const auto byte_count = std::min<std::size_t>(count * 2, transfer_bytes_.size());
  for (std::size_t index = 0; index < byte_count / 2; ++index) {
    transfer_bytes_[index * 2] = static_cast<std::uint8_t>(pixels[index] >> 8);
    transfer_bytes_[index * 2 + 1] = static_cast<std::uint8_t>(pixels[index]);
  }
  gpio_put(board::DisplayDc, 1);
  gpio_put(board::DisplayCs, 0);
  if (dma_channel_ >= 0) {
    auto config = dma_channel_get_default_config(dma_channel_);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_dreq(&config, spi_get_dreq(spi0, true));
    dma_channel_configure(dma_channel_, &config, &spi0_hw->dr,
                          transfer_bytes_.data(), byte_count, true);
    wait_for_transfer();
  } else {
    spi_write_blocking(spi0, transfer_bytes_.data(), byte_count);
  }
  gpio_put(board::DisplayCs, 1);
#else
  (void)count;
#endif
}

}  // namespace midi::display

