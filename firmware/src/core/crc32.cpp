#include "crc32.hpp"

namespace midi {
namespace {
std::uint32_t update(std::uint32_t crc, std::uint8_t byte) {
  crc ^= byte;
  for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1U) ^ ((crc & 1U) != 0U ? 0xedb88320U : 0U);
  return crc;
}
}  // namespace

std::uint32_t crc32(std::span<const std::byte> bytes) {
  std::uint32_t crc = 0xffffffffU;
  for (const std::byte byte : bytes) crc = update(crc, std::to_integer<std::uint8_t>(byte));
  return crc ^ 0xffffffffU;
}

std::uint32_t crc32_with_zeroed_range(std::span<const std::byte> bytes, std::size_t zeroOffset, std::size_t zeroLength) {
  std::uint32_t crc = 0xffffffffU;
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    const bool zero = index >= zeroOffset && index < zeroOffset + zeroLength;
    crc = update(crc, zero ? 0U : std::to_integer<std::uint8_t>(bytes[index]));
  }
  return crc ^ 0xffffffffU;
}

}  // namespace midi
