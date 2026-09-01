#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace midi {

enum class FrameError : std::uint8_t {
  CrcMismatch,
  UnsupportedVersion,
  PayloadTooLarge,
  UnknownCommand,
  InvalidFrame,
};

struct DecodedFrame {
  std::uint32_t request_id{};
  std::uint16_t command{};
  std::uint16_t flags{};
  std::array<std::byte, 4096> payload{};
  std::uint32_t payload_length{};
};

class FrameSink {
 public:
  virtual ~FrameSink() = default;
  virtual void on_frame(const DecodedFrame& frame) = 0;
  virtual void on_error(FrameError error) = 0;
};

class FrameDecoder {
 public:
  static constexpr std::size_t HeaderSize = 18;
  static constexpr std::size_t CrcSize = 4;
  static constexpr std::size_t MaxPayload = 4096;
  static constexpr std::size_t BufferSize = HeaderSize + MaxPayload + CrcSize;

  void feed(std::span<const std::byte> bytes, FrameSink& sink);
  void reset() { size_ = 0; }

 private:
  static std::uint16_t u16(const std::array<std::byte, BufferSize>& bytes, std::size_t at);
  static std::uint32_t u32(const std::array<std::byte, BufferSize>& bytes, std::size_t at);
  static bool is_command(std::uint16_t command);
  void resync();

  std::array<std::byte, BufferSize> buffer_{};
  std::size_t size_{};
};

}  // namespace midi
