#include "controller_protocol_service.hpp"

#include <algorithm>
#include <array>
#include <string_view>

#include "../core/image_reader.hpp"

namespace midi {

bool ControllerProtocolService::append_text(std::span<std::byte> output, std::size_t& at, std::string_view text) {
  if (text.size() > output.size() - at) return false;
  for (const auto character : text) output[at++] = static_cast<std::byte>(character);
  return true;
}

bool ControllerProtocolService::append_u32(std::span<std::byte> output, std::size_t& at, std::uint32_t value) {
  std::array<char, 10> digits{};
  std::size_t count = 0;
  do {
    digits[count++] = static_cast<char>('0' + (value % 10u));
    value /= 10u;
  } while (value != 0);
  if (count > output.size() - at) return false;
  while (count != 0) output[at++] = static_cast<std::byte>(digits[--count]);
  return true;
}

usb::ServiceError ControllerProtocolService::upload_error(bool accepted) {
  return accepted ? usb::ServiceError::None : usb::ServiceError::InvalidConfiguration;
}

usb::ServiceError ControllerProtocolService::capabilities(std::span<std::byte> output, std::size_t& size) {
  constexpr std::string_view json =
      R"({"deviceModel":"MIDI_PEDAL_PICO2","firmwareVersion":"0.1.0-wip","protocolVersion":1,"configSchema":1,"imageFormat":1,"queueCapacity":64,"destinations":["TRS","USB","BOTH"],"expression":true,"relays":2,"display":"ST7796S"})";
  size = 0;
  if (!append_text(output, size, json)) return usb::ServiceError::InvalidState;
  return usb::ServiceError::None;
}

usb::ServiceError ControllerProtocolService::config_info(std::span<std::byte> output, std::size_t& size) {
  size = 0;
  const auto info = store_.active_info();
  const auto slot = info.has_value() && info->slot == 1 ? "B" : "A";
  if (!append_text(output, size, R"({"sequence":)")) return usb::ServiceError::InvalidState;
  if (!append_u32(output, size, info.has_value() ? info->sequence : 0)) return usb::ServiceError::InvalidState;
  if (!append_text(output, size, R"(,"imageSize":)")) return usb::ServiceError::InvalidState;
  if (!append_u32(output, size, info.has_value() ? info->image_size : 0)) return usb::ServiceError::InvalidState;
  if (!append_text(output, size, R"(,"imageCrc32":)")) return usb::ServiceError::InvalidState;
  if (!append_u32(output, size, info.has_value() ? info->image_crc32 : 0)) return usb::ServiceError::InvalidState;
  if (!append_text(output, size, R"(,"activeSlot":")")) return usb::ServiceError::InvalidState;
  if (!append_text(output, size, slot)) return usb::ServiceError::InvalidState;
  if (!append_text(output, size, R"(","bankCount":)")) return usb::ServiceError::InvalidState;
  if (!append_u32(output, size, info.has_value() ? info->bank_count : 0)) return usb::ServiceError::InvalidState;
  if (!append_text(output, size, "}")) return usb::ServiceError::InvalidState;
  return usb::ServiceError::None;
}

usb::ServiceError ControllerProtocolService::read_config(std::uint8_t bank_index, std::span<std::byte> output, std::size_t& size) {
  return store_.read_active_bank_record(bank_index, output, size) ? usb::ServiceError::None : usb::ServiceError::InvalidConfiguration;
}

usb::ServiceError ControllerProtocolService::begin_upload(std::uint32_t image_size, std::uint32_t sequence, std::uint32_t image_crc32) {
  return upload_error(store_.begin_upload(image_size, sequence, image_crc32));
}

usb::ServiceError ControllerProtocolService::write_chunk(std::uint32_t offset, std::span<const std::byte> bytes) {
  return upload_error(store_.write_chunk(offset, bytes));
}

usb::ServiceError ControllerProtocolService::verify_upload() {
  return store_.verify_upload() ? usb::ServiceError::None : usb::ServiceError::VerifyFailed;
}

usb::ServiceError ControllerProtocolService::activate_upload() {
  return store_.activate_upload() ? usb::ServiceError::None : usb::ServiceError::InvalidState;
}

usb::ServiceError ControllerProtocolService::expression_sample(std::uint16_t& value) {
  value = expression_input_.read_adc();
  return usb::ServiceError::None;
}

usb::ServiceError ControllerProtocolService::set_expression_calibration(std::uint16_t heel, std::uint16_t toe) {
  return expression_.set_calibration({heel, toe}) ? usb::ServiceError::None : usb::ServiceError::InvalidConfiguration;
}

usb::ServiceError ControllerProtocolService::factory_empty_reset() {
  if (factory_image_.empty()) return usb::ServiceError::InvalidState;
  const auto image = ImageReader(factory_image_).inspect();
  if (image.error != ImageError::None) return usb::ServiceError::InvalidConfiguration;
  if (!store_.begin_upload(image.imageSize, image.sequence, image.crc32)) return usb::ServiceError::InvalidState;
  for (std::size_t offset = 0; offset < factory_image_.size(); offset += 1024) {
    const auto length = std::min<std::size_t>(1024, factory_image_.size() - offset);
    if (!store_.write_chunk(static_cast<std::uint32_t>(offset), factory_image_.subspan(offset, length))) return usb::ServiceError::InvalidState;
  }
  if (!store_.verify_upload()) return usb::ServiceError::VerifyFailed;
  return store_.activate_upload() ? usb::ServiceError::None : usb::ServiceError::InvalidState;
}

}  // namespace midi
