#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "../core/frame_decoder.hpp"

namespace midi::usb {

enum class Command : std::uint16_t {
  GetCapabilities = 1,
  GetConfigInfo = 2,
  ReadConfig = 3,
  BeginUpload = 4,
  WriteChunk = 5,
  VerifyUpload = 6,
  ActivateUpload = 7,
  GetExpressionSample = 8,
  SetExpressionCalibration = 9,
  FactoryEmptyReset = 10,
};

enum class ServiceError : std::uint8_t {
  None,
  InvalidFrame,
  IncompatibleDevice,
  InvalidConfiguration,
  InvalidState,
  VerifyFailed,
  Busy,
};

class ProtocolResponseSink {
 public:
  virtual ~ProtocolResponseSink() = default;
  [[nodiscard]] virtual bool write_frame(std::span<const std::byte> bytes) = 0;
};

class ProtocolService {
 public:
  virtual ~ProtocolService() = default;

  virtual ServiceError capabilities(std::span<std::byte> output, std::size_t& size) = 0;
  virtual ServiceError config_info(std::span<std::byte> output, std::size_t& size) = 0;
  virtual ServiceError read_config(std::uint8_t bank_index, std::span<std::byte> output, std::size_t& size) = 0;
  virtual ServiceError begin_upload(std::uint32_t image_size, std::uint32_t sequence, std::uint32_t image_crc32) = 0;
  virtual ServiceError write_chunk(std::uint32_t offset, std::span<const std::byte> bytes) = 0;
  virtual ServiceError verify_upload() = 0;
  virtual ServiceError activate_upload() = 0;
  virtual ServiceError expression_sample(std::uint16_t& value) = 0;
  virtual ServiceError set_expression_calibration(std::uint16_t heel, std::uint16_t toe) = 0;
  virtual ServiceError factory_empty_reset() = 0;
};

class ProtocolDispatcher final : private FrameSink {
 public:
  static constexpr std::size_t ResponsePayloadCapacity = FrameDecoder::MaxPayload;
  static constexpr std::size_t ResponseFrameCapacity = FrameDecoder::HeaderSize + ResponsePayloadCapacity + FrameDecoder::CrcSize;
  static constexpr std::size_t ResponseBodyCapacity = ResponsePayloadCapacity - 1;
  static constexpr std::size_t CacheCapacity = 16;

  ProtocolDispatcher(ProtocolService& service, ProtocolResponseSink& sink) : service_(service), sink_(sink) {}

  void receive(std::span<const std::byte> bytes) { decoder_.feed(bytes, *this); }
  [[nodiscard]] std::uint32_t decoder_errors() const { return decoder_errors_; }
  static const char* error_name(ServiceError error);

 private:
  struct CacheEntry {
    bool valid{};
    std::uint32_t request_id{};
    std::uint16_t command{};
    std::uint16_t flags{};
    std::uint32_t age{};
    std::size_t length{};
    std::array<std::byte, ResponseFrameCapacity> frame{};
  };

  void on_frame(const DecodedFrame& frame) override;
  void on_error(FrameError error) override;
  ServiceError dispatch(const DecodedFrame& frame, std::span<std::byte> output, std::size_t& size);
  bool replay(const DecodedFrame& frame);
  void emit(const DecodedFrame& request, ServiceError error, std::span<const std::byte> body);
  static std::uint16_t u16(std::span<const std::byte> bytes, std::size_t at);
  static std::uint32_t u32(std::span<const std::byte> bytes, std::size_t at);

  ProtocolService& service_;
  ProtocolResponseSink& sink_;
  FrameDecoder decoder_;
  std::array<std::byte, ResponseBodyCapacity> body_{};
  std::array<std::byte, ResponseFrameCapacity> frame_{};
  std::array<CacheEntry, CacheCapacity> cache_{};
  std::uint32_t age_{};
  std::uint32_t decoder_errors_{};
};

}  // namespace midi::usb
