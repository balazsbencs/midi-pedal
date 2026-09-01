#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <optional>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "core/config_store.hpp"
#include "core/crc32.hpp"
#include "core/expression.hpp"

namespace {
class FakeFlash final : public midi::FlashPort {
 public:
  static constexpr std::size_t Size = 4 * 1024 * 1024;
  FakeFlash() : bytes(Size, std::byte{0xff}) {}

  bool erase(std::uint32_t offset, std::size_t length) override {
    if (should_fail()) return false;
    if (offset + length > bytes.size()) return false;
    std::fill(bytes.begin() + offset, bytes.begin() + offset + length, std::byte{0xff});
    return true;
  }
  bool program(std::uint32_t offset, std::span<const std::byte> data) override {
    if (should_fail()) return false;
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

  void fail_after(std::size_t operation) { failAt = operation; operations = 0; }
  std::vector<std::byte> bytes;

 private:
  bool should_fail() { return failAt.has_value() && operations++ >= *failAt; }
  std::optional<std::size_t> failAt;
  std::size_t operations{};
};

std::vector<std::byte> fixture() {
  std::ifstream stream(std::string(MIDI_PEDAL_SOURCE_DIR) + "/protocol/fixtures/bin/minimal-valid.bin", std::ios::binary);
  const std::vector<char> raw{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
  std::vector<std::byte> output(raw.size());
  for (std::size_t index = 0; index < raw.size(); ++index) output[index] = static_cast<std::byte>(static_cast<unsigned char>(raw[index]));
  return output;
}

std::uint32_t u32(const std::vector<std::byte>& bytes, std::size_t at) {
  return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[at]) |
    (std::to_integer<unsigned char>(bytes[at + 1]) << 8u) |
    (std::to_integer<unsigned char>(bytes[at + 2]) << 16u) |
    (std::to_integer<unsigned char>(bytes[at + 3]) << 24u));
}

void upload_and_activate(midi::ConfigStore& store, const std::vector<std::byte>& image, std::uint32_t sequence) {
  ASSERT_TRUE(store.begin_upload(static_cast<std::uint32_t>(image.size()), sequence, u32(image, 28)));
  for (std::size_t offset = 0; offset < image.size(); offset += 1024) {
    const auto length = std::min<std::size_t>(1024, image.size() - offset);
    ASSERT_TRUE(store.write_chunk(static_cast<std::uint32_t>(offset), std::span<const std::byte>(image.data() + offset, length)));
  }
  ASSERT_TRUE(store.verify_upload());
  ASSERT_TRUE(store.activate_upload());
}

void upload_and_verify(midi::ConfigStore& store, const std::vector<std::byte>& image, std::uint32_t sequence) {
  ASSERT_TRUE(store.begin_upload(static_cast<std::uint32_t>(image.size()), sequence, u32(image, 28)));
  for (std::size_t offset = 0; offset < image.size(); offset += 1024) {
    const auto length = std::min<std::size_t>(1024, image.size() - offset);
    ASSERT_TRUE(store.write_chunk(static_cast<std::uint32_t>(offset), std::span<const std::byte>(image.data() + offset, length)));
  }
  ASSERT_TRUE(store.verify_upload());
}

std::vector<std::byte> with_sequence(std::vector<std::byte> image, std::uint8_t sequence) {
  image[12] = static_cast<std::byte>(sequence);
  const auto crc = midi::crc32_with_zeroed_range(image, 28, 4);
  image[28] = static_cast<std::byte>(crc & 0xffu);
  image[29] = static_cast<std::byte>((crc >> 8u) & 0xffu);
  image[30] = static_cast<std::byte>((crc >> 16u) & 0xffu);
  image[31] = static_cast<std::byte>((crc >> 24u) & 0xffu);
  return image;
}
}  // namespace

TEST(ConfigStore, ActivatesValidatedImageAndReportsMetadata) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  const auto image = fixture();
  upload_and_activate(store, image, 1);
  const auto info = store.active_info();
  ASSERT_TRUE(info.has_value());
  EXPECT_EQ(info->sequence, 1U);
  EXPECT_EQ(info->image_size, image.size());
  EXPECT_EQ(info->slot, 0U);
  midi::BankConfig bank{};
  EXPECT_TRUE(store.load_bank(0, bank));
  std::array<std::byte, 4096> record{};
  std::size_t record_size = 0;
  EXPECT_TRUE(store.read_active_bank_record(0, record, record_size));
  EXPECT_GT(record_size, 4U);
  EXPECT_EQ(std::to_integer<std::uint8_t>(record[0]), record_size & 0xffU);
}

TEST(ConfigStore, RejectsMisalignedOrOutOfBoundsChunks) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  const auto image = fixture();
  ASSERT_TRUE(store.begin_upload(static_cast<std::uint32_t>(image.size()), 1, u32(image, 28)));
  EXPECT_FALSE(store.write_chunk(1, std::span<const std::byte>(image.data(), 8)));
  EXPECT_FALSE(store.write_chunk(static_cast<std::uint32_t>(image.size()), std::span<const std::byte>(image.data(), 1)));
}

TEST(ConfigStore, PowerCutBeforeActivationKeepsPreviousSlot) {
  const auto image = fixture();
  for (std::size_t failAt = 0; failAt < 4; ++failAt) {
    FakeFlash flash;
    midi::ConfigStore first(flash);
    upload_and_activate(first, image, 1);
    auto newer = image;
    newer[12] = std::byte{2};
    const auto crc = midi::crc32_with_zeroed_range(newer, 28, 4);
    newer[28] = static_cast<std::byte>(crc & 0xffu);
    newer[29] = static_cast<std::byte>((crc >> 8u) & 0xffu);
    newer[30] = static_cast<std::byte>((crc >> 16u) & 0xffu);
    newer[31] = static_cast<std::byte>((crc >> 24u) & 0xffu);
    flash.fail_after(failAt);
    midi::ConfigStore interrupted(flash);
    if (interrupted.begin_upload(static_cast<std::uint32_t>(newer.size()), 2, crc)) {
      const auto chunk = std::span<const std::byte>(newer.data(), std::min<std::size_t>(1024, newer.size()));
      interrupted.write_chunk(0, chunk);
    }
    midi::ConfigStore rebooted(flash);
    const auto info = rebooted.active_info();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->sequence, 1U);
  }
}

TEST(ConfigStore, PersistsExpressionCalibrationWithoutAnActiveImageAcrossReboot) {
  FakeFlash flash;
  midi::ConfigStore first(flash);

  EXPECT_EQ(first.expression_calibration().heel, 0U);
  EXPECT_EQ(first.expression_calibration().toe, 4095U);
  ASSERT_TRUE(first.set_expression_calibration({120, 3900}));

  midi::ConfigStore rebooted(flash);
  EXPECT_EQ(rebooted.expression_calibration().heel, 120U);
  EXPECT_EQ(rebooted.expression_calibration().toe, 3900U);
  EXPECT_FALSE(rebooted.active_info().has_value());
}

TEST(ConfigStore, KeepsExpressionCalibrationWhenActivatingANewImage) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  const auto image = fixture();
  upload_and_activate(store, image, 1);
  ASSERT_TRUE(store.set_expression_calibration({180, 3800}));
  auto newer = image;
  newer[12] = std::byte{2};
  const auto crc = midi::crc32_with_zeroed_range(newer, 28, 4);
  newer[28] = static_cast<std::byte>(crc & 0xffu);
  newer[29] = static_cast<std::byte>((crc >> 8u) & 0xffu);
  newer[30] = static_cast<std::byte>((crc >> 16u) & 0xffu);
  newer[31] = static_cast<std::byte>((crc >> 24u) & 0xffu);

  upload_and_activate(store, newer, 2);

  midi::ConfigStore rebooted(flash);
  ASSERT_TRUE(rebooted.active_info().has_value());
  EXPECT_EQ(rebooted.active_info()->sequence, 2U);
  EXPECT_EQ(rebooted.expression_calibration().heel, 180U);
  EXPECT_EQ(rebooted.expression_calibration().toe, 3800U);
}

TEST(ConfigStore, PowerCutDuringCalibrationWriteKeepsThePreviousCalibration) {
  FakeFlash flash;
  midi::ConfigStore first(flash);
  ASSERT_TRUE(first.set_expression_calibration({100, 3900}));

  flash.fail_after(0);
  EXPECT_FALSE(first.set_expression_calibration({300, 3700}));

  midi::ConfigStore rebooted(flash);
  EXPECT_EQ(rebooted.expression_calibration().heel, 100U);
  EXPECT_EQ(rebooted.expression_calibration().toe, 3900U);
}

TEST(ConfigStore, FirstCalibrationLeavesARedundantRecordAfterEitherSectorIsCorrupted) {
  FakeFlash flash;
  midi::ConfigStore store(flash);
  ASSERT_TRUE(store.set_expression_calibration({120, 3900}));

  flash.bytes[midi::ConfigStore::MetadataAOffset + 32] = std::byte{0};
  midi::ConfigStore rebooted(flash);

  EXPECT_EQ(rebooted.expression_calibration().heel, 120U);
  EXPECT_EQ(rebooted.expression_calibration().toe, 3900U);
}

TEST(ConfigStore, FirstCalibrationRetainsARecoverableValueAtEveryWriteCut) {
  for (std::size_t fail_at = 0; fail_at < 4; ++fail_at) {
    FakeFlash flash;
    midi::ConfigStore store(flash);
    flash.fail_after(fail_at);

    EXPECT_FALSE(store.set_expression_calibration({120, 3900}));

    midi::ConfigStore rebooted(flash);
    if (fail_at < 2) {
      EXPECT_EQ(rebooted.expression_calibration().heel, 0U);
      EXPECT_EQ(rebooted.expression_calibration().toe, 4095U);
    } else {
      EXPECT_EQ(rebooted.expression_calibration().heel, 120U);
      EXPECT_EQ(rebooted.expression_calibration().toe, 3900U);
    }
  }
}

TEST(ConfigStore, ActiveImageCalibrationRetainsImageAndValueAtEveryWriteCut) {
  const auto image = fixture();
  for (std::size_t fail_at = 0; fail_at < 4; ++fail_at) {
    FakeFlash flash;
    midi::ConfigStore store(flash);
    upload_and_activate(store, image, 1);
    flash.fail_after(fail_at);

    EXPECT_FALSE(store.set_expression_calibration({120, 3900}));

    midi::ConfigStore rebooted(flash);
    ASSERT_TRUE(rebooted.active_info().has_value());
    EXPECT_EQ(rebooted.active_info()->sequence, 1U);
    if (fail_at < 2) {
      EXPECT_EQ(rebooted.expression_calibration().heel, 0U);
      EXPECT_EQ(rebooted.expression_calibration().toe, 4095U);
    } else {
      EXPECT_EQ(rebooted.expression_calibration().heel, 120U);
      EXPECT_EQ(rebooted.expression_calibration().toe, 3900U);
    }
  }
}

TEST(ConfigStore, ActivationKeepsASoleCurrentCalibrationAtEveryMetadataWriteCut) {
  const auto image = fixture();
  const auto newer = with_sequence(image, 2);
  for (std::size_t fail_at = 0; fail_at < 4; ++fail_at) {
    FakeFlash flash;
    midi::ConfigStore first(flash);
    upload_and_activate(first, image, 1);
    ASSERT_TRUE(first.set_expression_calibration({180, 3800}));
    flash.bytes[midi::ConfigStore::MetadataBOffset + 32] = std::byte{0};
    midi::ConfigStore store(flash);
    upload_and_verify(store, newer, 2);
    flash.fail_after(fail_at);

    EXPECT_FALSE(store.activate_upload());

    midi::ConfigStore rebooted(flash);
    ASSERT_TRUE(rebooted.active_info().has_value());
    EXPECT_EQ(rebooted.active_info()->sequence, 1U);
    EXPECT_EQ(rebooted.expression_calibration().heel, 180U);
    EXPECT_EQ(rebooted.expression_calibration().toe, 3800U);
  }
}
