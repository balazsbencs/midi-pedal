#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "config_types.hpp"

namespace midi {

enum class ImageError : std::uint8_t {
  None,
  Truncated,
  Magic,
  Version,
  Size,
  Layout,
  Crc,
  Offset,
  RecordLength,
  Enum,
  Count,
  String,
  Range,
};

struct ImageInspection {
  ImageError error{ImageError::None};
  std::uint16_t formatVersion{};
  std::uint32_t imageSize{};
  std::uint32_t sequence{};
  std::uint16_t bankCount{};
  std::uint32_t crc32{};
};

class ImageReader {
 public:
  explicit ImageReader(std::span<const std::byte> bytes) : bytes_(bytes) {}

  ImageInspection inspect() const;
  [[nodiscard]] bool validate_all_banks() const;
  [[nodiscard]] std::span<const std::byte> bank_record(std::uint8_t bankIndex) const;
  bool load_bank(std::uint8_t bankIndex, BankConfig& output) const;

 private:
  [[nodiscard]] std::span<const std::byte> bank_record_unchecked(std::uint8_t bank_index,
                                                                 std::uint16_t bank_count) const;
  std::span<const std::byte> bytes_;
};

}  // namespace midi
