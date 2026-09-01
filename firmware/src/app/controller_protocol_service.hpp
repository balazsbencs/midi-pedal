#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "../core/config_store.hpp"
#include "../core/expression.hpp"
#include "../usb/protocol_dispatcher.hpp"

namespace midi {

class ExpressionSampleInput {
 public:
  virtual ~ExpressionSampleInput() = default;
  [[nodiscard]] virtual std::uint16_t read_adc() const = 0;
};

class ControllerProtocolService final : public usb::ProtocolService {
 public:
  ControllerProtocolService(ConfigStore& store, ExpressionSampleInput& expression_input, ExpressionProcessor& expression,
                            std::span<const std::byte> factory_image = {})
      : store_(store), expression_input_(expression_input), expression_(expression), factory_image_(factory_image) {}

  usb::ServiceError capabilities(std::span<std::byte> output, std::size_t& size) override;
  usb::ServiceError config_info(std::span<std::byte> output, std::size_t& size) override;
  usb::ServiceError read_config(std::uint8_t bank_index, std::span<std::byte> output, std::size_t& size) override;
  usb::ServiceError begin_upload(std::uint32_t image_size, std::uint32_t sequence, std::uint32_t image_crc32) override;
  usb::ServiceError write_chunk(std::uint32_t offset, std::span<const std::byte> bytes) override;
  usb::ServiceError verify_upload() override;
  usb::ServiceError activate_upload() override;
  usb::ServiceError expression_sample(std::uint16_t& value) override;
  usb::ServiceError set_expression_calibration(std::uint16_t heel, std::uint16_t toe) override;
  usb::ServiceError factory_empty_reset() override;

 private:
  static bool append_text(std::span<std::byte> output, std::size_t& at, std::string_view text);
  static bool append_u32(std::span<std::byte> output, std::size_t& at, std::uint32_t value);
  static usb::ServiceError upload_error(bool accepted);

  ConfigStore& store_;
  ExpressionSampleInput& expression_input_;
  ExpressionProcessor& expression_;
  std::span<const std::byte> factory_image_{};
};

}  // namespace midi
