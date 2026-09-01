#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "../core/config_store.hpp"

namespace midi {

class PicoFlashPort final : public FlashPort {
 public:
  static constexpr std::size_t SectorSize = 4096;
  static constexpr std::size_t ProgramPageSize = 256;
  static constexpr std::size_t FlashSize = 4 * 1024 * 1024;

  bool erase(std::uint32_t offset, std::size_t length) override;
  bool program(std::uint32_t offset, std::span<const std::byte> data) override;
  void read(std::uint32_t offset, std::span<std::byte> output) const override;
  const std::byte* mapped(std::uint32_t offset, std::size_t length) const override;

 private:
  static bool in_bounds(std::uint32_t offset, std::size_t length);
};

}  // namespace midi
