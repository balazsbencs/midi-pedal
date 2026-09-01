#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "core/crc32.hpp"
#include "core/frame_decoder.hpp"

namespace {
std::vector<std::byte> frame(std::uint32_t request, std::uint16_t command, std::span<const std::byte> payload) {
  std::vector<std::byte> bytes(18 + payload.size() + 4);
  bytes[0] = std::byte{'M'}; bytes[1] = std::byte{'P'}; bytes[2] = std::byte{'C'}; bytes[3] = std::byte{'F'};
  auto put16 = [&](std::size_t at, std::uint16_t value) { bytes[at] = static_cast<std::byte>(value & 0xffu); bytes[at + 1] = static_cast<std::byte>(value >> 8u); };
  auto put32 = [&](std::size_t at, std::uint32_t value) { for (unsigned shift = 0; shift < 32; shift += 8) bytes[at + shift / 8] = static_cast<std::byte>(value >> shift); };
  put16(4, 1); put32(6, request); put16(10, command); put16(12, 0); put32(14, static_cast<std::uint32_t>(payload.size()));
  std::copy(payload.begin(), payload.end(), bytes.begin() + 18);
  const auto checksum = midi::crc32(std::span<const std::byte>(bytes.data(), 18 + payload.size()));
  put32(18 + payload.size(), checksum);
  return bytes;
}

struct Sink final : midi::FrameSink {
  std::vector<midi::DecodedFrame> frames;
  std::vector<midi::FrameError> errors;
  void on_frame(const midi::DecodedFrame& frame) override { frames.push_back(frame); }
  void on_error(midi::FrameError error) override { errors.push_back(error); }
};
}  // namespace

TEST(FrameDecoder, AcceptsNoiseAndArbitraryChunkBoundaries) {
  const std::array payload{std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
  const auto encoded = frame(42, 5, payload);
  Sink sink;
  midi::FrameDecoder decoder;
  decoder.feed(std::array{std::byte{0x00}, std::byte{0x7f}}, sink);
  for (std::size_t index = 0; index < encoded.size(); ++index) decoder.feed(std::span(encoded.data() + index, 1), sink);
  ASSERT_EQ(sink.frames.size(), 1U);
  EXPECT_EQ(sink.frames[0].request_id, 42U);
  EXPECT_EQ(sink.frames[0].command, 5U);
  EXPECT_EQ(sink.frames[0].payload_length, 3U);
  EXPECT_TRUE(sink.errors.empty());
}

TEST(FrameDecoder, ReportsBadCrcAndContinuesWithNextFrame) {
  const std::array<std::byte, 0> empty{};
  auto bad = frame(1, 1, empty);
  bad.back() ^= std::byte{0x01};
  const auto good = frame(2, 2, empty);
  bad.insert(bad.end(), good.begin(), good.end());
  Sink sink;
  midi::FrameDecoder decoder;
  decoder.feed(bad, sink);
  ASSERT_EQ(sink.errors.size(), 1U);
  EXPECT_EQ(sink.errors[0], midi::FrameError::CrcMismatch);
  ASSERT_EQ(sink.frames.size(), 1U);
  EXPECT_EQ(sink.frames[0].request_id, 2U);
}

TEST(FrameDecoder, RejectsUnsupportedVersionCommandAndPayload) {
  const std::array<std::byte, 0> empty{};
  auto unsupported = frame(1, 1, empty);
  unsupported[4] = std::byte{2};
  Sink sink;
  midi::FrameDecoder decoder;
  decoder.feed(unsupported, sink);
  EXPECT_EQ(sink.errors.back(), midi::FrameError::UnsupportedVersion);

  auto unknown = frame(2, 99, empty);
  decoder.feed(unknown, sink);
  EXPECT_EQ(sink.errors.back(), midi::FrameError::UnknownCommand);

  std::array<std::byte, 18> oversized{};
  oversized[0] = std::byte{'M'}; oversized[1] = std::byte{'P'}; oversized[2] = std::byte{'C'}; oversized[3] = std::byte{'F'};
  oversized[4] = std::byte{1}; oversized[14] = std::byte{0x01}; oversized[15] = std::byte{0x20};
  decoder.feed(oversized, sink);
  EXPECT_EQ(sink.errors.back(), midi::FrameError::PayloadTooLarge);
}
