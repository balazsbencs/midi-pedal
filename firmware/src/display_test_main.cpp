#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#include "board/pico2_pins.hpp"
#include "display/font_ascii.hpp"

namespace {

constexpr std::uint16_t ScreenWidth = 480;
constexpr std::uint16_t ScreenHeight = 320;
constexpr std::uint32_t SpiFrequencyHz = 8'000'000;

constexpr std::uint16_t Black = 0x0000;
constexpr std::uint16_t White = 0xFFFF;
constexpr std::uint16_t Red = 0xF800;
constexpr std::uint16_t Green = 0x07E0;
constexpr std::uint16_t Blue = 0x001F;
constexpr std::uint16_t Yellow = 0xFFE0;
constexpr std::uint16_t Cyan = 0x07FF;
constexpr std::uint16_t Magenta = 0xF81F;
constexpr std::uint16_t DarkGray = 0x2104;

void write_command(std::uint8_t command,
                   std::span<const std::uint8_t> parameters = {}) {
  gpio_put(midi::board::DisplayCs, 0);
  gpio_put(midi::board::DisplayDc, 0);
  spi_write_blocking(spi0, &command, 1);
  if (!parameters.empty()) {
    gpio_put(midi::board::DisplayDc, 1);
    spi_write_blocking(spi0, parameters.data(), parameters.size());
  }
  gpio_put(midi::board::DisplayCs, 1);
}

template <std::size_t Size>
void write_command(std::uint8_t command,
                   const std::array<std::uint8_t, Size>& parameters) {
  write_command(command, std::span<const std::uint8_t>(parameters));
}

void initialize_display() {
  gpio_init(midi::board::DisplayCs);
  gpio_init(midi::board::DisplayDc);
  gpio_init(midi::board::DisplayReset);
  gpio_put(midi::board::DisplayCs, 1);
  gpio_put(midi::board::DisplayDc, 1);
  gpio_put(midi::board::DisplayReset, 1);
  gpio_set_dir(midi::board::DisplayCs, GPIO_OUT);
  gpio_set_dir(midi::board::DisplayDc, GPIO_OUT);
  gpio_set_dir(midi::board::DisplayReset, GPIO_OUT);

  spi_init(spi0, SpiFrequencyHz);
  spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_set_function(midi::board::DisplaySck, GPIO_FUNC_SPI);
  gpio_set_function(midi::board::DisplayMosi, GPIO_FUNC_SPI);

  sleep_ms(20);
  gpio_put(midi::board::DisplayReset, 0);
  sleep_ms(120);
  gpio_put(midi::board::DisplayReset, 1);
  sleep_ms(200);

  write_command(0x01);  // Software reset.
  sleep_ms(150);

  // Conservative ST7796S initialization based on the controller vendor
  // sequence used by known-good reference drivers.
  write_command(0xF0, std::array<std::uint8_t, 1>{0xC3});
  write_command(0xF0, std::array<std::uint8_t, 1>{0x96});
  write_command(0xC5, std::array<std::uint8_t, 1>{0x1C});
  write_command(0x36, std::array<std::uint8_t, 1>{0x28});  // Landscape, BGR.
  write_command(0x3A, std::array<std::uint8_t, 1>{0x55});  // RGB565.
  write_command(0xB0, std::array<std::uint8_t, 1>{0x80});
  write_command(0xB4, std::array<std::uint8_t, 1>{0x00});
  write_command(0xB6, std::array<std::uint8_t, 3>{0x80, 0x02, 0x3B});
  write_command(0xB7, std::array<std::uint8_t, 1>{0xC6});
  write_command(0xF0, std::array<std::uint8_t, 1>{0x69});
  write_command(0xF0, std::array<std::uint8_t, 1>{0x3C});

  write_command(0x11);  // Sleep out.
  sleep_ms(150);
  write_command(0x13);  // Normal display mode.
  write_command(0x21);  // Panel inversion on (normal for this controller).
  write_command(0x29);  // Display on.
  sleep_ms(150);
}

void begin_full_screen_write() {
  write_command(0x2A, std::array<std::uint8_t, 4>{0x00, 0x00, 0x01, 0xDF});
  write_command(0x2B, std::array<std::uint8_t, 4>{0x00, 0x00, 0x01, 0x3F});

  const std::uint8_t memory_write = 0x2C;
  gpio_put(midi::board::DisplayCs, 0);
  gpio_put(midi::board::DisplayDc, 0);
  spi_write_blocking(spi0, &memory_write, 1);
  gpio_put(midi::board::DisplayDc, 1);
}

void end_full_screen_write() {
  gpio_put(midi::board::DisplayCs, 1);
}

void fill_screen(std::uint16_t color) {
  std::array<std::uint8_t, ScreenWidth * 2> row{};
  for (std::size_t x = 0; x < ScreenWidth; ++x) {
    row[x * 2] = static_cast<std::uint8_t>(color >> 8);
    row[x * 2 + 1] = static_cast<std::uint8_t>(color);
  }

  begin_full_screen_write();
  for (std::size_t y = 0; y < ScreenHeight; ++y) {
    spi_write_blocking(spi0, row.data(), row.size());
  }
  end_full_screen_write();
}

bool text_pixel(std::uint16_t x, std::uint16_t y, std::int32_t origin_x,
                std::int32_t origin_y, std::uint8_t scale,
                std::string_view text) {
  const auto local_x = static_cast<std::int32_t>(x) - origin_x;
  const auto local_y = static_cast<std::int32_t>(y) - origin_y;
  if (local_x < 0 || local_y < 0 || local_y >= 7 * scale) return false;

  const auto character = static_cast<std::size_t>(local_x / (6 * scale));
  if (character >= text.size()) return false;
  const auto column = static_cast<std::uint8_t>((local_x % (6 * scale)) / scale);
  if (column >= 5) return false;
  const auto row = static_cast<std::uint8_t>(local_y / scale);
  const auto glyph = midi::display::glyph_for(text[character]);
  return (glyph[row] & (1u << (4u - column))) != 0;
}

void draw_test_card() {
  constexpr std::array<std::uint16_t, 8> bars{
      White, Yellow, Cyan, Green, Magenta, Red, Blue, Black};
  std::array<std::uint8_t, ScreenWidth * 2> row{};

  begin_full_screen_write();
  for (std::uint16_t y = 0; y < ScreenHeight; ++y) {
    for (std::uint16_t x = 0; x < ScreenWidth; ++x) {
      std::uint16_t color = y < 176 ? bars[x / 60] : DarkGray;
      const bool border = x < 4 || x >= ScreenWidth - 4 || y < 4 ||
                          y >= ScreenHeight - 4;
      const bool title = text_pixel(x, y, 96, 205, 4, "DISPLAY TEST");
      const bool details =
          text_pixel(x, y, 78, 263, 3, "SPI0 8 MHZ RGB565");
      if (border || title || details) color = White;
      row[x * 2] = static_cast<std::uint8_t>(color >> 8);
      row[x * 2 + 1] = static_cast<std::uint8_t>(color);
    }
    spi_write_blocking(spi0, row.data(), row.size());
  }
  end_full_screen_write();
}

}  // namespace

int main() {
  stdio_init_all();
  initialize_display();

  // The startup flashes make the three color channels and repeated full-frame
  // writes easy to verify before the static test card appears.
  fill_screen(Red);
  sleep_ms(500);
  fill_screen(Green);
  sleep_ms(500);
  fill_screen(Blue);
  sleep_ms(500);
  draw_test_card();

  while (true) tight_loop_contents();
}
