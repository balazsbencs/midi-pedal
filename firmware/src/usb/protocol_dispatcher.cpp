#include "protocol_dispatcher.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>

#include "../core/crc32.hpp"

namespace midi::usb {
namespace {
constexpr std::array<std::byte, 4> Magic{std::byte{'M'}, std::byte{'P'}, std::byte{'C'}, std::byte{'F'}};
constexpr std::uint16_t Version = 1;

void put_u16(std::span<std::byte> bytes, std::size_t at, std::uint16_t value) {
  bytes[at] = static_cast<std::byte>(value & 0xffu);
  bytes[at + 1] = static_cast<std::byte>(value >> 8u);
}

void put_u32(std::span<std::byte> bytes, std::size_t at, std::uint32_t value) {
  for (unsigned shift = 0; shift < 32; shift += 8) bytes[at + shift / 8] = static_cast<std::byte>(value >> shift);
}

std::size_t error_payload(ServiceError error, std::span<std::byte> output) {
  const auto* name = ProtocolDispatcher::error_name(error);
  const auto prefix = std::string_view{"{\"code\":\""};
  const auto suffix = std::string_view{"\"}"};
  const auto name_length = std::strlen(name);
  if (prefix.size() + name_length + suffix.size() > output.size()) return 0;
  std::memcpy(output.data(), prefix.data(), prefix.size());
  std::memcpy(output.data() + prefix.size(), name, name_length);
  std::memcpy(output.data() + prefix.size() + name_length, suffix.data(), suffix.size());
  return prefix.size() + name_length + suffix.size();
}
}  // namespace

std::uint16_t ProtocolDispatcher::u16(std::span<const std::byte> bytes, std::size_t at) {
  return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[at]) |
    (std::to_integer<std::uint8_t>(bytes[at + 1]) << 8u));
}

std::uint32_t ProtocolDispatcher::u32(std::span<const std::byte> bytes, std::size_t at) {
  return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[at]) |
    (std::to_integer<std::uint8_t>(bytes[at + 1]) << 8u) |
    (std::to_integer<std::uint8_t>(bytes[at + 2]) << 16u) |
    (std::to_integer<std::uint8_t>(bytes[at + 3]) << 24u));
}

const char* ProtocolDispatcher::error_name(ServiceError error) {
  switch (error) {
    case ServiceError::None: return "OK";
    case ServiceError::InvalidFrame: return "INVALID_FRAME";
    case ServiceError::IncompatibleDevice: return "INCOMPATIBLE_DEVICE";
    case ServiceError::InvalidConfiguration: return "INVALID_CONFIGURATION";
    case ServiceError::InvalidState: return "INVALID_STATE";
    case ServiceError::VerifyFailed: return "VERIFY_FAILED";
    case ServiceError::Busy: return "BUSY";
  }
  return "INVALID_STATE";
}

void ProtocolDispatcher::on_error(FrameError) {
  ++decoder_errors_;
  // A malformed frame does not carry a trustworthy request id, so there is no
  // response frame that the editor could safely correlate with this error.
}

bool ProtocolDispatcher::replay(const DecodedFrame& frame) {
  for (auto& entry : cache_) {
    if (!entry.valid || entry.request_id != frame.request_id || entry.command != frame.command || entry.flags != frame.flags) continue;
    entry.age = ++age_;
    // The command was already executed. A transient transport failure must
    // not cause a retry with a second side effect; the host owns retry timing.
    (void)sink_.write_frame(std::span<const std::byte>(entry.frame.data(), entry.length));
    return true;
  }
  return false;
}

ServiceError ProtocolDispatcher::dispatch(const DecodedFrame& frame, std::span<std::byte> output, std::size_t& size) {
  size = 0;
  if (frame.flags != 0) return ServiceError::InvalidFrame;
  const auto payload = std::span<const std::byte>(frame.payload.data(), frame.payload_length);
  switch (static_cast<Command>(frame.command)) {
    case Command::GetCapabilities:
      if (!payload.empty()) return ServiceError::InvalidFrame;
      return service_.capabilities(output, size);
    case Command::GetConfigInfo:
      if (!payload.empty()) return ServiceError::InvalidFrame;
      return service_.config_info(output, size);
    case Command::ReadConfig:
      if (payload.size() != 1) return ServiceError::InvalidFrame;
      return service_.read_config(std::to_integer<std::uint8_t>(payload[0]), output, size);
    case Command::BeginUpload:
      if (payload.size() != 12) return ServiceError::InvalidFrame;
      return service_.begin_upload(u32(payload, 0), u32(payload, 4), u32(payload, 8));
    case Command::WriteChunk:
      if (payload.size() < 4 || payload.size() > 1028) return ServiceError::InvalidFrame;
      return service_.write_chunk(u32(payload, 0), payload.subspan(4));
    case Command::VerifyUpload:
      if (!payload.empty()) return ServiceError::InvalidFrame;
      return service_.verify_upload();
    case Command::ActivateUpload:
      if (!payload.empty()) return ServiceError::InvalidFrame;
      return service_.activate_upload();
    case Command::GetExpressionSample: {
      if (!payload.empty() || output.size() < 14) return ServiceError::InvalidFrame;
      std::uint16_t value{};
      const auto error = service_.expression_sample(value);
      if (error != ServiceError::None) return error;
      constexpr std::string_view prefix{"{\"value\":"};
      constexpr std::string_view suffix{"}"};
      const auto hundreds = static_cast<char>('0' + ((value / 100) % 10));
      const auto tens = static_cast<char>('0' + ((value / 10) % 10));
      const auto ones = static_cast<char>('0' + (value % 10));
      std::size_t at = 0;
      for (const auto character : prefix) output[at++] = static_cast<std::byte>(character);
      if (value >= 100) output[at++] = static_cast<std::byte>(hundreds);
      if (value >= 10) output[at++] = static_cast<std::byte>(tens);
      output[at++] = static_cast<std::byte>(ones);
      for (const auto character : suffix) output[at++] = static_cast<std::byte>(character);
      size = at;
      return ServiceError::None;
    }
    case Command::SetExpressionCalibration:
      if (payload.size() != 4) return ServiceError::InvalidFrame;
      return service_.set_expression_calibration(u16(payload, 0), u16(payload, 2));
    case Command::FactoryEmptyReset:
      if (!payload.empty()) return ServiceError::InvalidFrame;
      return service_.factory_empty_reset();
  }
  return ServiceError::InvalidFrame;
}

void ProtocolDispatcher::emit(const DecodedFrame& request, ServiceError error, std::span<const std::byte> body) {
  const auto body_size = error == ServiceError::None ? body.size() : error_payload(error, body_);
  const auto payload_size = body_size + 1;
  if (payload_size > ResponsePayloadCapacity) return;
  frame_[0] = Magic[0]; frame_[1] = Magic[1]; frame_[2] = Magic[2]; frame_[3] = Magic[3];
  put_u16(frame_, 4, Version);
  put_u32(frame_, 6, request.request_id);
  put_u16(frame_, 10, request.command);
  put_u16(frame_, 12, 0);
  put_u32(frame_, 14, static_cast<std::uint32_t>(payload_size));
  frame_[18] = error == ServiceError::None ? std::byte{0} : std::byte{1};
  if (body_size != 0) {
    const auto source = error == ServiceError::None ? body : std::span<const std::byte>(body_.data(), body_size);
    std::copy(source.begin(), source.end(), frame_.begin() + 19);
  }
  put_u32(frame_, 18 + payload_size, crc32(std::span<const std::byte>(frame_.data(), 18 + payload_size)));
  const auto length = 18 + payload_size + FrameDecoder::CrcSize;

  std::size_t replacement = 0;
  for (std::size_t index = 0; index < cache_.size(); ++index) {
    if (!cache_[index].valid) { replacement = index; break; }
    if (cache_[index].age < cache_[replacement].age) replacement = index;
  }
  auto& entry = cache_[replacement];
  entry.valid = true;
  entry.request_id = request.request_id;
  entry.command = request.command;
  entry.flags = request.flags;
  entry.age = ++age_;
  entry.length = length;
  std::copy_n(frame_.begin(), length, entry.frame.begin());
  sink_.write_frame(std::span<const std::byte>(frame_.data(), length));
}

void ProtocolDispatcher::on_frame(const DecodedFrame& frame) {
  if (replay(frame)) return;
  std::size_t size = 0;
  const auto error = dispatch(frame, body_, size);
  if (error == ServiceError::None && size > body_.size()) {
    emit(frame, ServiceError::InvalidState, {});
    return;
  }
  emit(frame, error, std::span<const std::byte>(body_.data(), size));
}

}  // namespace midi::usb
