#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace midi {

std::uint32_t crc32(std::span<const std::byte> bytes);
std::uint32_t crc32_with_zeroed_range(std::span<const std::byte> bytes, std::size_t zeroOffset, std::size_t zeroLength);

}  // namespace midi
