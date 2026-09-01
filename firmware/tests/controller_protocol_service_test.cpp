#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "app/controller_protocol_service.hpp"
#include "core/config_store.hpp"
#include "core/crc32.hpp"
#include "core/expression.hpp"

namespace {
class FakeFlash final : public midi::FlashPort {
 public:
  static constexpr std::size_t Size = 4 * 1024 * 1024;
  FakeFlash() : bytes(Size, std::byte{0xff}) {}

  bool erase(std::uint32_t offset, std::size_t length) override {
    if (offset + length > bytes.size()) return false;
    std::fill(bytes.begin() + offset, bytes.begin() + offset + length, std::byte{0xff});
    return true;
  }
  bool program(std::uint32_t offset, std::span<const std::byte> data) override {
    if (offset + data.size() > bytes.size()) return false;
    for (std::size_t index = 0; index < data.size(); ++index) bytes[offset + index] &= data[index];
    return true;
  }
  void read(std::uint32_t offset, std::span<std::byte> output) const override {
    if (offset + output.size() > bytes.size()) { std::fill(output.begin(), output.end(), std::byte{0}); return; }
    std::copy_n(bytes.begin() + offset, output.size(), output.begin());
  }
  const std::byte* mapped(std::uint32_t offset, std::size_t length) const override {
    return offset + length <= bytes.size() ? bytes.data() + offset : nullptr;
  }

  std::vector<std::byte> bytes;
};

class FakeExpressionInput final : public midi::ExpressionSampleInput {
 public:
  std::uint16_t value{2048};
  std::uint16_t read_adc() const override { return value; }
};

std::vector<std::byte> fixture() {
  std::ifstream stream(std::string(MIDI_PEDAL_SOURCE_DIR) + "/protocol/fixtures/bin/minimal-valid.bin", std::ios::binary);
  const std::vector<char> raw{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
  std::vector<std::byte> output(raw.size());
  for (std::size_t index = 0; index < raw.size(); ++index) output[index] = static_cast<std::byte>(static_cast<unsigned char>(raw[index]));
  return output;
}

std::uint32_t u32(std::span<const std::byte> bytes, std::size_t at) {
  return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[at]) |
    (std::to_integer<std::uint8_t>(bytes[at + 1]) << 8u) |
    (std::to_integer<std::uint8_t>(bytes[at + 2]) << 16u) |
    (std::to_integer<std::uint8_t>(bytes[at + 3]) << 24u));
}

void activate(midi::ConfigStore& store, std::span<const std::byte> image) {
  ASSERT_TRUE(store.begin_upload(static_cast<std::uint32_t>(image.size()), 1, u32(image, 28)));
  for (std::size_t offset = 0; offset < image.size(); offset += 1024) {
    ASSERT_TRUE(store.write_chunk(static_cast<std::uint32_t>(offset), image.subspan(offset, std::min<std::size_t>(1024, image.size() - offset))));
  }
  ASSERT_TRUE(store.verify_upload());
  ASSERT_TRUE(store.activate_upload());
}
}  // namespace

TEST(ControllerProtocolService, ReportsCapabilitiesAndActiveMetadata) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  const auto image = fixture();
  activate(store, image);
  FakeExpressionInput input;
  midi::ExpressionProcessor expression({0, 4095});
  midi::ControllerProtocolService service(store, input, expression);

  std::array<std::byte, 4095> output{};
  std::size_t size = 0;
  ASSERT_EQ(service.capabilities(output, size), midi::usb::ServiceError::None);
  const std::string capabilities(reinterpret_cast<const char*>(output.data()), size);
  EXPECT_NE(capabilities.find("MIDI_PEDAL_PICO2"), std::string::npos);
  EXPECT_NE(capabilities.find("ST7796S"), std::string::npos);

  ASSERT_EQ(service.config_info(output, size), midi::usb::ServiceError::None);
  const std::string metadata(reinterpret_cast<const char*>(output.data()), size);
  EXPECT_NE(metadata.find("\"sequence\":1"), std::string::npos);
  EXPECT_NE(metadata.find("\"bankCount\":1"), std::string::npos);
}

TEST(ControllerProtocolService, ReadsValidatedBankRecordAndSamplesExpression) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  const auto image = fixture();
  activate(store, image);
  FakeExpressionInput input;
  midi::ExpressionProcessor expression({0, 4095});
  midi::ControllerProtocolService service(store, input, expression);

  std::array<std::byte, 4095> output{};
  std::size_t size = 0;
  ASSERT_EQ(service.read_config(0, output, size), midi::usb::ServiceError::None);
  EXPECT_EQ(size, u32(output, 0));
  EXPECT_GT(size, 32U);

  std::uint16_t value = 0;
  ASSERT_EQ(service.expression_sample(value), midi::usb::ServiceError::None);
  EXPECT_EQ(value, 2048U);
  EXPECT_EQ(service.set_expression_calibration(3000, 2000), midi::usb::ServiceError::InvalidConfiguration);
  EXPECT_EQ(service.set_expression_calibration(100, 2000), midi::usb::ServiceError::None);
}

TEST(ControllerProtocolService, RejectsUploadWhenConfigStoreRejectsIt) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  FakeExpressionInput input;
  midi::ExpressionProcessor expression({0, 4095});
  midi::ControllerProtocolService service(store, input, expression);
  EXPECT_EQ(service.begin_upload(0, 1, 0), midi::usb::ServiceError::InvalidConfiguration);
  EXPECT_EQ(service.verify_upload(), midi::usb::ServiceError::VerifyFailed);
  EXPECT_EQ(service.activate_upload(), midi::usb::ServiceError::InvalidState);
}

TEST(ControllerProtocolService, FactoryResetValidatesAndActivatesEmbeddedImage) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  FakeExpressionInput input;
  midi::ExpressionProcessor expression({0, 4095});
  const auto image = fixture();
  midi::ControllerProtocolService service(store, input, expression, image);

  EXPECT_EQ(service.factory_empty_reset(), midi::usb::ServiceError::None);
  const auto info = store.active_info();
  ASSERT_TRUE(info.has_value());
  EXPECT_EQ(info->image_size, image.size());
  EXPECT_EQ(info->image_crc32, u32(image, 28));
  EXPECT_EQ(info->bank_count, 128U);
}
