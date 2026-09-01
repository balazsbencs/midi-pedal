#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstring>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "core/image_reader.hpp"
#include "core/crc32.hpp"

namespace {
std::vector<std::byte> read_fixture(const char* name) {
  std::ifstream stream(std::filesystem::path(MIDI_PEDAL_SOURCE_DIR) / "protocol" / "fixtures" / "bin" / name, std::ios::binary);
  const std::vector<char> raw{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
  std::vector<std::byte> bytes(raw.size());
  std::memcpy(bytes.data(), raw.data(), raw.size());
  return bytes;
}

void write_u32(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
  bytes[offset] = static_cast<std::byte>(value & 0xffU);
  bytes[offset + 1] = static_cast<std::byte>((value >> 8U) & 0xffU);
  bytes[offset + 2] = static_cast<std::byte>((value >> 16U) & 0xffU);
  bytes[offset + 3] = static_cast<std::byte>((value >> 24U) & 0xffU);
}
}  // namespace

TEST(ImageReader, LoadsGoldenBankWithoutAllocation) {
  auto bytes = read_fixture("minimal-valid.bin");
  midi::ImageReader reader(std::span<const std::byte>(bytes.data(), bytes.size()));
  ASSERT_EQ(reader.inspect().error, midi::ImageError::None);
  midi::BankConfig bank{};
  ASSERT_TRUE(reader.load_bank(0, bank));
  EXPECT_EQ(bank.pages[0].presets.size(), 4U);
}

TEST(ImageReader, RejectsTruncatedHeaderAndLeavesOutputZeroed) {
  auto bytes = read_fixture("minimal-valid.bin");
  bytes.resize(31);
  midi::ImageReader reader(std::span<const std::byte>(bytes.data(), bytes.size()));
  EXPECT_EQ(reader.inspect().error, midi::ImageError::Truncated);
  midi::BankConfig bank{};
  bank.id = 99;
  EXPECT_FALSE(reader.load_bank(0, bank));
  EXPECT_EQ(bank.id, 0U);
}

TEST(ImageReader, RejectsBadCrcAndBadOffset) {
  auto bytes = read_fixture("minimal-valid.bin");
  bytes[bytes.size() - 1] ^= std::byte{0x01};
  EXPECT_EQ(midi::ImageReader(std::span<const std::byte>(bytes.data(), bytes.size())).inspect().error, midi::ImageError::Crc);

  auto offsetBytes = read_fixture("minimal-valid.bin");
  offsetBytes[32] = std::byte{0xff};
  offsetBytes[33] = std::byte{0xff};
  offsetBytes[34] = std::byte{0xff};
  offsetBytes[35] = std::byte{0xff};
  write_u32(offsetBytes, 28, midi::crc32_with_zeroed_range(std::span<const std::byte>(offsetBytes.data(), offsetBytes.size()), 28, 4));
  midi::BankConfig bank{};
  EXPECT_FALSE(midi::ImageReader(std::span<const std::byte>(offsetBytes.data(), offsetBytes.size())).load_bank(0, bank));
}
