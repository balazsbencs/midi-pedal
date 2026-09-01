#include "frame_decoder.hpp"

#include <algorithm>
#include <cstring>

#include "crc32.hpp"

namespace midi {
namespace {
constexpr std::array<std::byte, 4> Magic{std::byte{'M'}, std::byte{'P'}, std::byte{'C'}, std::byte{'F'}};
constexpr std::uint16_t Version = 1;
}  // namespace

std::uint16_t FrameDecoder::u16(const std::array<std::byte, BufferSize>& bytes, std::size_t at) {
  return static_cast<std::uint16_t>(std::to_integer<unsigned char>(bytes[at]) | (std::to_integer<unsigned char>(bytes[at + 1]) << 8u));
}

std::uint32_t FrameDecoder::u32(const std::array<std::byte, BufferSize>& bytes, std::size_t at) {
  return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[at]) |
    (std::to_integer<unsigned char>(bytes[at + 1]) << 8u) |
    (std::to_integer<unsigned char>(bytes[at + 2]) << 16u) |
    (std::to_integer<unsigned char>(bytes[at + 3]) << 24u));
}

bool FrameDecoder::is_command(std::uint16_t command) { return command >= 1 && command <= 10; }

void FrameDecoder::resync() {
  std::size_t start = 1;
  while (start + Magic.size() <= size_) {
    if (std::equal(Magic.begin(), Magic.end(), buffer_.begin() + static_cast<std::ptrdiff_t>(start))) {
      std::memmove(buffer_.data(), buffer_.data() + start, size_ - start);
      size_ -= start;
      return;
    }
    ++start;
  }
  const auto keep = std::min<std::size_t>(size_, Magic.size() - 1);
  if (keep != 0) std::memmove(buffer_.data(), buffer_.data() + size_ - keep, keep);
  size_ = keep;
}

void FrameDecoder::feed(std::span<const std::byte> bytes, FrameSink& sink) {
  for (const auto byte : bytes) {
    if (size_ == buffer_.size()) resync();
    buffer_[size_++] = byte;
    while (size_ >= Magic.size() && !std::equal(Magic.begin(), Magic.end(), buffer_.begin())) resync();
    if (size_ < HeaderSize) continue;

    const auto payload_length = u32(buffer_, 14);
    if (payload_length > MaxPayload) {
      sink.on_error(FrameError::PayloadTooLarge);
      resync();
      continue;
    }
    const auto frame_length = HeaderSize + static_cast<std::size_t>(payload_length) + CrcSize;
    if (size_ < frame_length) continue;

    const auto version = u16(buffer_, 4);
    const auto command = u16(buffer_, 10);
    if (version != Version) sink.on_error(FrameError::UnsupportedVersion);
    else if (!is_command(command)) sink.on_error(FrameError::UnknownCommand);
    else {
      const auto expected = u32(buffer_, HeaderSize + payload_length);
      const auto actual = crc32(std::span<const std::byte>(buffer_.data(), HeaderSize + payload_length));
      if (actual != expected) sink.on_error(FrameError::CrcMismatch);
      else {
        DecodedFrame frame{};
        frame.request_id = u32(buffer_, 6);
        frame.command = command;
        frame.flags = u16(buffer_, 12);
        frame.payload_length = payload_length;
        std::copy_n(buffer_.begin() + HeaderSize, payload_length, frame.payload.begin());
        sink.on_frame(frame);
      }
    }
    std::memmove(buffer_.data(), buffer_.data() + frame_length, size_ - frame_length);
    size_ -= frame_length;
  }
}

}  // namespace midi
