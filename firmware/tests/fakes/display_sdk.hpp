#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

// A narrow SDK fake for executing the display driver's device branch on host.
namespace display_sdk {
inline bool cs_high = true;
inline bool dc_high = true;
inline bool dma_active = false;
inline bool wire_busy = false;
inline bool premature_cs = false;
inline unsigned busy_checks = 0;
inline const std::uint8_t* source = nullptr;
inline std::size_t count = 0;
inline std::vector<std::uint8_t> pixel_bytes;
inline unsigned windows = 0;
inline unsigned dma_starts = 0;
inline unsigned blocking_dma_waits = 0;
inline void reset() {
  cs_high = dc_high = true;
  dma_active = wire_busy = premature_cs = false;
  busy_checks = windows = dma_starts = blocking_dma_waits = 0;
  pixel_bytes.clear();
}
}

inline constexpr unsigned SPI_CPOL_0 = 0, SPI_CPHA_0 = 0, SPI_MSB_FIRST = 0;
inline void spi_set_format(void*, unsigned, unsigned, unsigned, unsigned) {}
inline constexpr unsigned GPIO_FUNC_SPI = 1, GPIO_OUT = 1, DMA_SIZE_8 = 0;
struct spi_inst_t {};
inline spi_inst_t instance;
inline spi_inst_t* spi0 = &instance;
struct spi_hw_t { volatile std::uint32_t dr{}; };
inline spi_hw_t registers;
inline spi_hw_t* spi0_hw = &registers;
struct dma_channel_config {};
inline void gpio_init(unsigned) {}
inline void gpio_set_function(unsigned, unsigned) {}
inline void gpio_set_dir(unsigned, unsigned) {}
inline void gpio_put(unsigned pin, bool high) {
  if (pin == 17) {
    if (high && (display_sdk::dma_active || display_sdk::wire_busy)) display_sdk::premature_cs = true;
    display_sdk::cs_high = high;
  }
  if (pin == 20) display_sdk::dc_high = high;
}
inline unsigned spi_init(spi_inst_t*, unsigned rate) { return rate; }
inline int spi_write_blocking(spi_inst_t*, const std::uint8_t* bytes, std::size_t size) {
  if (!display_sdk::dc_high && size == 1 && bytes[0] == 0x2C) ++display_sdk::windows;
  return static_cast<int>(size);
}
inline unsigned spi_get_dreq(spi_inst_t*, bool) { return 0; }
inline bool spi_is_busy(spi_inst_t*) {
  ++display_sdk::busy_checks;
  if (display_sdk::wire_busy) {
    display_sdk::wire_busy = false;
    return true;
  }
  return false;
}
inline int dma_claim_unused_channel(bool) { return 0; }
inline dma_channel_config dma_channel_get_default_config(int) { return {}; }
inline void channel_config_set_transfer_data_size(dma_channel_config*, unsigned) {}
inline void channel_config_set_dreq(dma_channel_config*, unsigned) {}
inline void dma_channel_configure(int, const dma_channel_config*, volatile void*,
                                  const void* source, std::size_t count, bool) {
  ++display_sdk::dma_starts;
  display_sdk::source = static_cast<const std::uint8_t*>(source);
  display_sdk::count = count;
  display_sdk::dma_active = true;
}
inline bool dma_channel_is_busy(int) { return display_sdk::dma_active; }
inline void complete_dma() {
  if (!display_sdk::dma_active) return;
  display_sdk::pixel_bytes.insert(display_sdk::pixel_bytes.end(), display_sdk::source,
                                  display_sdk::source + display_sdk::count);
  display_sdk::dma_active = false;
  display_sdk::wire_busy = true;
}
inline void dma_channel_wait_for_finish_blocking(int) {
  ++display_sdk::blocking_dma_waits;
  complete_dma();
}
inline void sleep_ms(unsigned) {}
inline void tight_loop_contents() {}
