#include "config_store.hpp"

#include <algorithm>
#include <array>

#include "crc32.hpp"
#include "image_reader.hpp"

namespace midi {
namespace {
constexpr std::size_t MetadataSize = 32;
constexpr std::size_t SettingsSize = 32;
constexpr std::size_t SettingsOffset = MetadataSize;

void put_u16(std::span<std::byte> bytes, std::size_t at, std::uint16_t value) {
  bytes[at] = static_cast<std::byte>(value & 0xffu);
  bytes[at + 1] = static_cast<std::byte>((value >> 8u) & 0xffu);
}

void put_u32(std::span<std::byte> bytes, std::size_t at, std::uint32_t value) {
  for (unsigned shift = 0; shift < 32; shift += 8) bytes[at + shift / 8] = static_cast<std::byte>((value >> shift) & 0xffu);
}

std::uint16_t get_u16(std::span<const std::byte> bytes, std::size_t at) {
  return static_cast<std::uint16_t>(std::to_integer<unsigned char>(bytes[at]) | (std::to_integer<unsigned char>(bytes[at + 1]) << 8u));
}

std::uint32_t get_u32(std::span<const std::byte> bytes, std::size_t at) {
  return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[at]) |
    (std::to_integer<unsigned char>(bytes[at + 1]) << 8u) |
    (std::to_integer<unsigned char>(bytes[at + 2]) << 16u) |
    (std::to_integer<unsigned char>(bytes[at + 3]) << 24u));
}

}  // namespace

void ConfigStore::write_metadata_record(std::span<std::byte> bytes, const Metadata& metadata) {
  if (!metadata.valid) return;
  std::fill(bytes.begin(), bytes.end(), std::byte{0});
  bytes[0] = std::byte{'M'}; bytes[1] = std::byte{'P'}; bytes[2] = std::byte{'M'}; bytes[3] = std::byte{'D'};
  put_u16(bytes, 4, 1);
  put_u32(bytes, 8, metadata.generation);
  bytes[12] = static_cast<std::byte>(metadata.slot);
  put_u32(bytes, 16, metadata.sequence);
  put_u32(bytes, 20, metadata.image_size);
  put_u32(bytes, 24, metadata.image_crc32);
  put_u32(bytes, 28, crc32_with_zeroed_range(bytes, 28, 4));
}

void ConfigStore::write_settings_record(std::span<std::byte> bytes, const Settings& settings) {
  if (!settings.valid) return;
  std::fill(bytes.begin(), bytes.end(), std::byte{0});
  bytes[0] = std::byte{'M'}; bytes[1] = std::byte{'P'}; bytes[2] = std::byte{'S'}; bytes[3] = std::byte{'T'};
  put_u16(bytes, 4, 1);
  put_u32(bytes, 8, settings.generation);
  put_u16(bytes, 12, settings.calibration.heel);
  put_u16(bytes, 14, settings.calibration.toe);
  put_u32(bytes, 28, crc32_with_zeroed_range(bytes, 28, 4));
}

ConfigStore::ConfigStore(FlashPort& flash) : flash_(flash) { scan(); }

ConfigStore::Metadata ConfigStore::read_metadata(std::uint8_t index) const {
  std::array<std::byte, MetadataSize> bytes{};
  flash_.read(metadata_offset(index), bytes);
  Metadata metadata{};
  if (std::to_integer<char>(bytes[0]) != 'M' || std::to_integer<char>(bytes[1]) != 'P' ||
      std::to_integer<char>(bytes[2]) != 'M' || std::to_integer<char>(bytes[3]) != 'D') return metadata;
  if (get_u16(bytes, 4) != 1 || get_u32(bytes, 28) != crc32_with_zeroed_range(bytes, 28, 4)) return metadata;
  metadata.generation = get_u32(bytes, 8);
  metadata.slot = std::to_integer<std::uint8_t>(bytes[12]);
  metadata.sequence = get_u32(bytes, 16);
  metadata.image_size = get_u32(bytes, 20);
  metadata.image_crc32 = get_u32(bytes, 24);
  metadata.valid = metadata.slot < 2 && metadata.image_size > 0 && metadata.image_size <= SlotSize;
  return metadata;
}

ConfigStore::Settings ConfigStore::read_settings(std::uint8_t index) const {
  std::array<std::byte, SettingsSize> bytes{};
  flash_.read(metadata_offset(index) + SettingsOffset, bytes);
  Settings settings{};
  if (std::to_integer<char>(bytes[0]) != 'M' || std::to_integer<char>(bytes[1]) != 'P' ||
      std::to_integer<char>(bytes[2]) != 'S' || std::to_integer<char>(bytes[3]) != 'T') return settings;
  if (get_u16(bytes, 4) != 1 || get_u32(bytes, 28) != crc32_with_zeroed_range(bytes, 28, 4)) return settings;
  settings.generation = get_u32(bytes, 8);
  settings.calibration = {get_u16(bytes, 12), get_u16(bytes, 14)};
  settings.valid = valid_calibration(settings.calibration);
  return settings;
}

bool ConfigStore::valid_calibration(Calibration calibration) {
  return calibration.toe > calibration.heel && calibration.toe - calibration.heel >= 410u && calibration.toe <= 4095u;
}

std::optional<std::uint8_t> ConfigStore::newest_settings_index() const {
  std::optional<std::uint8_t> selected;
  for (std::uint8_t index = 0; index < 2; ++index) {
    if (settings_[index].valid &&
        (!selected.has_value() || newer(settings_[index].generation, settings_[*selected].generation))) selected = index;
  }
  return selected;
}

bool ConfigStore::image_valid(std::uint8_t slot, std::uint32_t image_size, std::uint32_t sequence, std::uint32_t image_crc32) const {
  if (slot > 1 || image_size < 32 || image_size > SlotSize || image_size > MaxImageSize) return false;
  const auto* mapped = flash_.mapped(slot_offset(slot), image_size);
  if (mapped == nullptr) return false;
  const ImageReader reader(std::span<const std::byte>(mapped, image_size));
  const auto inspection = reader.inspect();
  if (inspection.error != ImageError::None || inspection.sequence != sequence || inspection.crc32 != image_crc32) return false;
  for (std::uint8_t index = 0; index < inspection.bankCount; ++index) {
    BankConfig bank{};
    if (!reader.load_bank(index, bank)) return false;
  }
  return true;
}

void ConfigStore::scan() {
  metadata_[0] = read_metadata(0);
  metadata_[1] = read_metadata(1);
  settings_[0] = read_settings(0);
  settings_[1] = read_settings(1);
  const auto selected_settings = newest_settings_index();
  expression_calibration_ = selected_settings.has_value() ? settings_[*selected_settings].calibration : Calibration{0, 4095};
  active_.reset();
  std::optional<std::uint8_t> selected;
  for (std::uint8_t index = 0; index < 2; ++index) {
    if (!metadata_[index].valid || !image_valid(metadata_[index].slot, metadata_[index].image_size, metadata_[index].sequence, metadata_[index].image_crc32)) continue;
    if (!selected.has_value() || newer(metadata_[index].generation, metadata_[*selected].generation)) selected = index;
  }
  if (!selected.has_value()) return;
  active_metadata_index_ = *selected;
  const auto& metadata = metadata_[*selected];
  const auto* mapped = flash_.mapped(slot_offset(metadata.slot), metadata.image_size);
  const auto inspection = mapped == nullptr ? ImageInspection{ImageError::Truncated} : ImageReader(std::span<const std::byte>(mapped, metadata.image_size)).inspect();
  active_ = ActiveImageInfo{metadata.slot, metadata.generation, metadata.sequence, metadata.image_size, metadata.image_crc32, inspection.bankCount};
}

bool ConfigStore::begin_upload(std::uint32_t image_size, std::uint32_t sequence, std::uint32_t image_crc32) {
  if (image_size == 0 || image_size > SlotSize || image_size > MaxImageSize) return false;
  const auto target_slot = active_.has_value() ? static_cast<std::uint8_t>(active_->slot ^ 1u) : 0u;
  upload_ = {};
  upload_.active = true;
  upload_.slot = target_slot;
  upload_.image_size = image_size;
  upload_.sequence = sequence;
  upload_.image_crc32 = image_crc32;
  return true;
}

bool ConfigStore::write_chunk(std::uint32_t offset, std::span<const std::byte> bytes) {
  if (!upload_.active || bytes.empty() || bytes.size() > UploadChunkMaxSize || offset >= upload_.image_size || offset + bytes.size() > upload_.image_size || (offset % 256u) != 0) return false;
  upload_.verified = false;
  if (!erase_upload_sectors(offset, bytes.size())) return false;
  return flash_.program(slot_offset(upload_.slot) + offset, bytes);
}

bool ConfigStore::erase_upload_sectors(std::uint32_t offset, std::size_t length) {
  const auto first_sector = static_cast<std::size_t>(offset / MetadataSectorSize);
  const auto last_sector = static_cast<std::size_t>((offset + length - 1u) / MetadataSectorSize);
  if (last_sector >= UploadSectorCount) return false;
  for (std::size_t sector = first_sector; sector <= last_sector; ++sector) {
    const auto word = sector / 32u;
    const auto bit = static_cast<std::uint32_t>(1u << (sector % 32u));
    if ((upload_.erased_sectors[word] & bit) != 0) continue;
    // Erase only one 4 KiB sector; PicoFlashPort disables interrupts around
    // this individual operation rather than around an entire image slot.
    if (!flash_.erase(slot_offset(upload_.slot) + static_cast<std::uint32_t>(sector * MetadataSectorSize),
                      MetadataSectorSize)) return false;
    upload_.erased_sectors[word] |= bit;
  }
  return true;
}

bool ConfigStore::verify_upload() {
  if (!upload_.active) return false;
  if (!image_valid(upload_.slot, upload_.image_size, upload_.sequence, upload_.image_crc32)) return false;
  upload_.verified = true;
  return true;
}

bool ConfigStore::write_metadata(std::uint8_t index, const Metadata& metadata) {
  return write_sector(index, metadata, settings_[index]);
}

bool ConfigStore::write_sector(std::uint8_t index, const Metadata& metadata, const Settings& settings) {
  std::array<std::byte, MetadataSize + SettingsSize> bytes{};
  bytes.fill(std::byte{0xff});
  write_metadata_record(std::span<std::byte>(bytes.data(), MetadataSize), metadata);
  write_settings_record(std::span<std::byte>(bytes.data() + SettingsOffset, SettingsSize), settings);
  if (!flash_.erase(metadata_offset(index), MetadataSectorSize)) return false;
  return flash_.program(metadata_offset(index), bytes);
}

bool ConfigStore::set_expression_calibration(Calibration calibration) {
  if (!valid_calibration(calibration)) return false;
  const auto selected = newest_settings_index();
  const auto next_generation = selected.has_value() ? settings_[*selected].generation + 1u : 1u;
  const Settings next{true, next_generation, calibration};

  if (active_.has_value()) {
    const auto active_index = active_metadata_index_;
    const auto peer_index = static_cast<std::uint8_t>(active_index ^ 1u);
    const auto active_metadata = metadata_[active_index];
    if (!write_sector(peer_index, active_metadata, next)) return false;
    metadata_[peer_index] = active_metadata;
    settings_[peer_index] = next;
    if (!write_sector(active_index, active_metadata, next)) return false;
    settings_[active_index] = next;
  } else {
    const auto first_index = selected.has_value() ? static_cast<std::uint8_t>(*selected ^ 1u) : 0u;
    const auto second_index = static_cast<std::uint8_t>(first_index ^ 1u);
    if (!write_sector(first_index, metadata_[first_index], next)) return false;
    settings_[first_index] = next;
    if (!write_sector(second_index, metadata_[second_index], next)) return false;
    settings_[second_index] = next;
  }
  expression_calibration_ = calibration;
  return true;
}

bool ConfigStore::activate_upload() {
  if (!upload_.active || !upload_.verified) return false;
  const auto next_generation = active_.has_value() ? active_->generation + 1u : 1u;
  std::uint8_t metadata_index = 0;
  if (metadata_[0].valid && metadata_[1].valid) metadata_index = newer(metadata_[0].generation, metadata_[1].generation) ? 1 : 0;
  else if (metadata_[0].valid) metadata_index = 1;
  else if (metadata_[1].valid) metadata_index = 0;
  const Metadata next{true, next_generation, upload_.slot, upload_.sequence, upload_.image_size, upload_.image_crc32};
  const auto peer_index = static_cast<std::uint8_t>(metadata_index ^ 1u);
  const auto selected_settings = newest_settings_index();
  const Settings effective_settings = selected_settings.has_value()
      ? settings_[*selected_settings]
      : Settings{true, 1, expression_calibration_};

  const auto target_has_current_fallback = metadata_[metadata_index].valid &&
      image_valid(metadata_[metadata_index].slot, metadata_[metadata_index].image_size,
                  metadata_[metadata_index].sequence, metadata_[metadata_index].image_crc32) &&
      (!active_.has_value() ||
       (metadata_[metadata_index].slot == metadata_[active_metadata_index_].slot &&
        metadata_[metadata_index].sequence == metadata_[active_metadata_index_].sequence &&
        metadata_[metadata_index].image_size == metadata_[active_metadata_index_].image_size &&
        metadata_[metadata_index].image_crc32 == metadata_[active_metadata_index_].image_crc32)) &&
      settings_[metadata_index].valid &&
      settings_[metadata_index].generation == effective_settings.generation &&
      settings_[metadata_index].calibration.heel == effective_settings.calibration.heel &&
      settings_[metadata_index].calibration.toe == effective_settings.calibration.toe;
  if (active_.has_value() && active_metadata_index_ == peer_index && !target_has_current_fallback) {
    const auto active_metadata = metadata_[active_metadata_index_];
    if (!write_sector(metadata_index, active_metadata, effective_settings)) return false;
    metadata_[metadata_index] = active_metadata;
    settings_[metadata_index] = effective_settings;
  }
  const auto peer_metadata = active_.has_value() ? metadata_[active_metadata_index_] : metadata_[peer_index];
  if (!write_sector(peer_index, peer_metadata, effective_settings)) return false;
  metadata_[peer_index] = peer_metadata;
  settings_[peer_index] = effective_settings;
  if (!write_sector(metadata_index, next, effective_settings)) return false;
  metadata_[metadata_index] = next;
  settings_[metadata_index] = effective_settings;
  upload_ = {};
  scan();
  return active_.has_value() && active_->sequence == next.sequence && active_->slot == next.slot;
}

bool ConfigStore::load_bank(std::uint8_t bank_index, BankConfig& output) const {
  output = {};
  if (!active_.has_value()) return false;
  const auto* mapped = flash_.mapped(slot_offset(active_->slot), active_->image_size);
  if (mapped == nullptr) return false;
  return ImageReader(std::span<const std::byte>(mapped, active_->image_size)).load_bank(bank_index, output);
}

bool ConfigStore::read_active_bank_record(std::uint8_t bank_index, std::span<std::byte> output, std::size_t& size) const {
  size = 0;
  if (!active_.has_value()) return false;
  const auto* mapped = flash_.mapped(slot_offset(active_->slot), active_->image_size);
  if (mapped == nullptr) return false;
  const auto record = ImageReader(std::span<const std::byte>(mapped, active_->image_size)).bank_record(bank_index);
  if (record.empty()) return false;
  if (output.size() < record.size()) return false;
  size = record.size();
  std::copy(record.begin(), record.end(), output.begin());
  return true;
}

}  // namespace midi
