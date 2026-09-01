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
  bool load_bank(std::uint8_t bankIndex, BankConfig& output) const;

 private:
  std::span<const std::byte> bytes_;
};

}  // namespace midi
