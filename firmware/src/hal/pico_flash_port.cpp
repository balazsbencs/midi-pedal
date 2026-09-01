#include "pico_flash_port.hpp"

#include <algorithm>
#include <array>
#include <cstring>

#ifdef PICO_ON_DEVICE

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/xip_cache.h"
#include "pico/platform.h"

namespace midi {

bool PicoFlashPort::in_bounds(std::uint32_t offset, std::size_t length) {
  return offset <= FlashSize && length <= FlashSize - offset;
}

bool PicoFlashPort::erase(std::uint32_t offset, std::size_t length) {
  if (!in_bounds(offset, length) || offset % SectorSize != 0 || length % SectorSize != 0 || length == 0) return false;
  const auto interrupts = save_and_disable_interrupts();
  flash_range_erase(offset, length);
  restore_interrupts(interrupts);
  return true;
}

bool PicoFlashPort::program(std::uint32_t offset, std::span<const std::byte> data) {
  if (!in_bounds(offset, data.size()) || data.empty()) return false;
  std::array<std::byte, ProgramPageSize> page{};
  std::size_t consumed = 0;
  while (consumed < data.size()) {
    const auto absolute = static_cast<std::size_t>(offset) + consumed;
    const auto page_offset = absolute % ProgramPageSize;
    const auto page_start = absolute - page_offset;
    if (page_start + ProgramPageSize > FlashSize) return false;
    const auto count = std::min(ProgramPageSize - page_offset, data.size() - consumed);
    std::fill(page.begin(), page.end(), std::byte{0xff});
    std::memcpy(page.data(), reinterpret_cast<const void*>(XIP_BASE + page_start), page.size());
    for (std::size_t index = 0; index < count; ++index) page[page_offset + index] &= data[consumed + index];
    const auto interrupts = save_and_disable_interrupts();
    flash_range_program(static_cast<std::uint32_t>(page_start), reinterpret_cast<const std::uint8_t*>(page.data()), page.size());
    restore_interrupts(interrupts);
    consumed += count;
  }
  return true;
}

void PicoFlashPort::read(std::uint32_t offset, std::span<std::byte> output) const {
  if (!in_bounds(offset, output.size())) { std::fill(output.begin(), output.end(), std::byte{0}); return; }
  std::memcpy(output.data(), reinterpret_cast<const void*>(XIP_BASE + offset), output.size());
}

const std::byte* PicoFlashPort::mapped(std::uint32_t offset, std::size_t length) const {
  return in_bounds(offset, length) ? reinterpret_cast<const std::byte*>(XIP_BASE + offset) : nullptr;
}

}  // namespace midi

#else

namespace midi {

bool PicoFlashPort::in_bounds(std::uint32_t offset, std::size_t length) {
  return offset <= FlashSize && length <= FlashSize - offset;
}
bool PicoFlashPort::erase(std::uint32_t, std::size_t) { return false; }
bool PicoFlashPort::program(std::uint32_t, std::span<const std::byte>) { return false; }
void PicoFlashPort::read(std::uint32_t, std::span<std::byte> output) const { std::fill(output.begin(), output.end(), std::byte{0}); }
const std::byte* PicoFlashPort::mapped(std::uint32_t, std::size_t) const { return nullptr; }

}  // namespace midi

#endif
