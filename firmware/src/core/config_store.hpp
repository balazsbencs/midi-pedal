#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "config_types.hpp"
#include "expression.hpp"

namespace midi {

class FlashPort {
 public:
  virtual ~FlashPort() = default;
  virtual bool erase(std::uint32_t offset, std::size_t length) = 0;
  virtual bool program(std::uint32_t offset, std::span<const std::byte> data) = 0;
  virtual void read(std::uint32_t offset, std::span<std::byte> output) const = 0;
  virtual const std::byte* mapped(std::uint32_t offset, std::size_t length) const = 0;
};

struct ActiveImageInfo {
  std::uint8_t slot{};
  std::uint32_t generation{};
  std::uint32_t sequence{};
  std::uint32_t image_size{};
  std::uint32_t image_crc32{};
  std::uint16_t bank_count{};
};

class ConfigStore {
 public:
  static constexpr std::uint32_t SlotAOffset = 0x200000;
  static constexpr std::uint32_t SlotBOffset = 0x2f0000;
  static constexpr std::size_t SlotSize = 0xf0000;
  static constexpr std::uint32_t MetadataAOffset = 0x3e0000;
  static constexpr std::uint32_t MetadataBOffset = 0x3f0000;
  static constexpr std::size_t MetadataSectorSize = 0x1000;
  static constexpr std::size_t MaxImageSize = 768 * 1024;

  explicit ConfigStore(FlashPort& flash);

  [[nodiscard]] std::optional<ActiveImageInfo> active_info() const { return active_; }
  bool begin_upload(std::uint32_t image_size, std::uint32_t sequence, std::uint32_t image_crc32);
  bool write_chunk(std::uint32_t offset, std::span<const std::byte> bytes);
  bool verify_upload();
  bool activate_upload();
  bool load_bank(std::uint8_t bank_index, BankConfig& output) const;
  bool read_active_bank_record(std::uint8_t bank_index, std::span<std::byte> output, std::size_t& size) const;
  [[nodiscard]] Calibration expression_calibration() const { return expression_calibration_; }
  bool set_expression_calibration(Calibration calibration);

 private:
  struct Upload {
    bool active{};
    bool verified{};
    std::uint8_t slot{};
    std::uint32_t image_size{};
    std::uint32_t sequence{};
    std::uint32_t image_crc32{};
  };

  struct Metadata {
    bool valid{};
    std::uint32_t generation{};
    std::uint8_t slot{};
    std::uint32_t sequence{};
    std::uint32_t image_size{};
    std::uint32_t image_crc32{};
  };

  struct Settings {
    bool valid{};
    std::uint32_t generation{};
    Calibration calibration{};
  };

  void scan();
  Metadata read_metadata(std::uint8_t index) const;
  Settings read_settings(std::uint8_t index) const;
  bool image_valid(std::uint8_t slot, std::uint32_t image_size, std::uint32_t sequence, std::uint32_t image_crc32) const;
  bool write_metadata(std::uint8_t index, const Metadata& metadata);
  bool write_sector(std::uint8_t index, const Metadata& metadata, const Settings& settings);
  static void write_metadata_record(std::span<std::byte> bytes, const Metadata& metadata);
  static void write_settings_record(std::span<std::byte> bytes, const Settings& settings);
  static bool valid_calibration(Calibration calibration);
  static std::uint32_t slot_offset(std::uint8_t slot) { return slot == 0 ? SlotAOffset : SlotBOffset; }
  static std::uint32_t metadata_offset(std::uint8_t index) { return index == 0 ? MetadataAOffset : MetadataBOffset; }
  static bool newer(std::uint32_t left, std::uint32_t right) { return static_cast<std::int32_t>(left - right) > 0; }

  FlashPort& flash_;
  std::optional<ActiveImageInfo> active_;
  std::array<Metadata, 2> metadata_{};
  std::array<Settings, 2> settings_{};
  Calibration expression_calibration_{0, 4095};
  std::uint8_t active_metadata_index_{};
  Upload upload_{};
};

}  // namespace midi
