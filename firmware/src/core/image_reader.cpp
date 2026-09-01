#include "image_reader.hpp"

#include <algorithm>
#include <array>
#include <cstring>

#include "crc32.hpp"

namespace midi {
namespace {
constexpr std::size_t kHeaderSize = 32;
constexpr std::size_t kIndexSize = 128 * 4;
constexpr std::size_t kPayloadOffset = kHeaderSize + kIndexSize;
constexpr std::size_t kMaxImageSize = 768 * 1024;
constexpr std::uint32_t kEmptyOffset = 0xffffffffU;

std::uint16_t get_u16(std::span<const std::byte> bytes, std::size_t offset) {
  return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset]) | (std::to_integer<std::uint8_t>(bytes[offset + 1]) << 8U));
}

std::uint32_t get_u32(std::span<const std::byte> bytes, std::size_t offset) {
  return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset]) |
    (std::to_integer<std::uint8_t>(bytes[offset + 1]) << 8U) |
    (std::to_integer<std::uint8_t>(bytes[offset + 2]) << 16U) |
    (std::to_integer<std::uint8_t>(bytes[offset + 3]) << 24U));
}

class Cursor {
 public:
  explicit Cursor(std::span<const std::byte> bytes) : bytes_(bytes) {}

  std::size_t position() const { return position_; }
  bool done() const { return position_ == bytes_.size(); }
  bool u8(std::uint8_t& result) {
    if (position_ >= bytes_.size()) return false;
    result = std::to_integer<std::uint8_t>(bytes_[position_++]); return true;
  }
  bool u16(std::uint16_t& result) {
    std::uint8_t low{}, high{}; if (!u8(low) || !u8(high)) return false;
    result = static_cast<std::uint16_t>(low | (high << 8U)); return true;
  }
  bool u32(std::uint32_t& result) {
    std::uint8_t b0{}, b1{}, b2{}, b3{}; if (!u8(b0) || !u8(b1) || !u8(b2) || !u8(b3)) return false;
    result = static_cast<std::uint32_t>(b0 | (b1 << 8U) | (b2 << 16U) | (b3 << 24U)); return true;
  }
  template <std::size_t Capacity>
  bool string(AsciiString<Capacity>& result) {
    std::uint8_t length{}; if (!u8(length) || length > Capacity || position_ + length > bytes_.size()) return false;
    result = {};
    result.length = length;
    for (std::size_t index = 0; index < length; ++index) {
      const auto byte = std::to_integer<std::uint8_t>(bytes_[position_++]);
      if (byte < 0x20U || byte > 0x7eU) return false;
      result.data[index] = static_cast<char>(byte);
    }
    result.data[length] = '\0';
    return true;
  }

 private:
  std::span<const std::byte> bytes_;
  std::size_t position_{};
};

bool destination(std::uint8_t value, Destination& output) {
  if (value > 2) return false; output = static_cast<Destination>(value); return true;
}
bool trigger(std::uint8_t value, Trigger& output) {
  if (value > 3) return false; output = static_cast<Trigger>(value); return true;
}
bool position_filter(std::uint8_t value, PositionFilter& output) {
  if (value > 2) return false; output = static_cast<PositionFilter>(value); return true;
}

bool read_message(Cursor& cursor, Message& output) {
  std::uint8_t type{}; if (!cursor.u8(type)) return false; output = {};
  if (type == 0) {
    output.kind = MessageKind::Pc;
    std::uint8_t destinationValue{}; if (!cursor.u8(output.channel) || !cursor.u8(output.data1) || !cursor.u8(destinationValue) || !destination(destinationValue, output.destination)) return false;
    return output.channel >= 1 && output.channel <= 16 && output.data1 <= 127;
  }
  if (type == 1) {
    output.kind = MessageKind::Cc;
    std::uint8_t destinationValue{}; if (!cursor.u8(output.channel) || !cursor.u8(output.data1) || !cursor.u8(output.data2) || !cursor.u8(destinationValue) || !destination(destinationValue, output.destination)) return false;
    return output.channel >= 1 && output.channel <= 16 && output.data1 <= 127 && output.data2 <= 127;
  }
  if (type == 2) {
    output.kind = MessageKind::Relay;
    if (!cursor.u8(output.contact) || !cursor.u8(output.operation)) return false;
    return (output.contact == 1 || output.contact == 2) && output.operation <= 2;
  }
  if (type == 3) {
    output.kind = MessageKind::Navigation;
    if (!cursor.u8(output.operation) || !cursor.u8(output.target) || output.operation > 5) return false;
    const bool bankSet = output.operation == 2;
    const bool pageSet = output.operation == 5;
    if (bankSet) return output.target >= 1 && output.target <= 128;
    if (pageSet) return output.target >= 1 && output.target <= 4;
    return output.target == 0;
  }
  return false;
}

bool read_slot(Cursor& cursor, MessageSlot& output) {
  std::uint8_t triggerValue{}, positionValue{};
  if (!cursor.u32(output.id) || output.id == 0 || !cursor.u8(triggerValue) || !cursor.u8(positionValue)) return false;
  return trigger(triggerValue, output.trigger) && position_filter(positionValue, output.position) && read_message(cursor, output.message);
}

bool read_preset(Cursor& cursor, Preset& output) {
  std::uint8_t toggle{}, slotCount{};
  if (!cursor.u32(output.id) || output.id == 0 || !cursor.string(output.position1.label) || !cursor.u16(output.position1.accentRgb565) ||
      !cursor.string(output.position2.label) || !cursor.u16(output.position2.accentRgb565) || !cursor.u8(toggle) || !cursor.u8(slotCount) || slotCount > 8) return false;
  output.toggleOn = toggle;
  output.slotCount = slotCount;
  if (toggle != 255 && toggle > 3) return false;
  for (std::size_t index = 0; index < slotCount; ++index) if (!read_slot(cursor, output.slots[index])) return false;
  return true;
}

bool read_bank_record(std::span<const std::byte> record, BankConfig& output) {
  Cursor cursor(record); std::uint32_t recordLength{}, pageId{}; std::uint8_t enabled{};
  if (!cursor.u32(recordLength) || recordLength != record.size() || !cursor.u32(output.id) || !cursor.string(output.name) ||
      !cursor.u8(enabled) || !cursor.string(output.expression.label) ||
      !cursor.u8(output.expression.channel) || !cursor.u8(output.expression.controller)) return false;
  output.expression.enabled = enabled != 0;
  std::uint8_t destinationValue{}, minimum{}, maximum{}, inverted{};
  if (!cursor.u8(destinationValue) || !destination(destinationValue, output.expression.destination) || !cursor.u8(minimum) || !cursor.u8(maximum) || !cursor.u8(inverted)) return false;
  output.expression.minimum = minimum; output.expression.maximum = maximum; output.expression.inverted = inverted != 0;
  if (output.id == 0 || output.expression.channel < 1 || output.expression.channel > 16 || output.expression.controller > 127 || minimum > maximum) return false;
  for (auto& page : output.pages) {
    if (!cursor.u32(pageId)) return false; page.id = pageId;
    for (auto& preset : page.presets) if (!read_preset(cursor, preset)) return false;
  }
  return cursor.done();
}
}  // namespace

ImageInspection ImageReader::inspect() const {
  ImageInspection result{};
  if (bytes_.size() < kPayloadOffset) { result.error = ImageError::Truncated; return result; }
  if (std::to_integer<char>(bytes_[0]) != 'M' || std::to_integer<char>(bytes_[1]) != 'P' || std::to_integer<char>(bytes_[2]) != 'D' || std::to_integer<char>(bytes_[3]) != 'L') { result.error = ImageError::Magic; return result; }
  result.formatVersion = get_u16(bytes_, 4); const auto headerSize = get_u16(bytes_, 6); result.imageSize = get_u32(bytes_, 8); result.sequence = get_u32(bytes_, 12); result.bankCount = get_u16(bytes_, 16); result.crc32 = get_u32(bytes_, 28);
  if (result.formatVersion != 1 || headerSize != kHeaderSize) { result.error = ImageError::Version; return result; }
  if (result.imageSize != bytes_.size() || result.imageSize > kMaxImageSize) { result.error = ImageError::Size; return result; }
  if (result.bankCount > 128 || get_u32(bytes_, 20) != kHeaderSize || get_u32(bytes_, 24) != kPayloadOffset) { result.error = ImageError::Layout; return result; }
  if (crc32_with_zeroed_range(bytes_, 28, 4) != result.crc32) { result.error = ImageError::Crc; return result; }
  return result;
}

bool ImageReader::load_bank(std::uint8_t bankIndex, BankConfig& output) const {
  output = {};
  const auto inspection = inspect();
  if (inspection.error != ImageError::None || bankIndex >= inspection.bankCount) return false;
  const auto relativeOffset = get_u32(bytes_, kHeaderSize + static_cast<std::size_t>(bankIndex) * 4);
  if (relativeOffset == kEmptyOffset || relativeOffset >= bytes_.size() - kPayloadOffset) return false;
  const auto recordStart = kPayloadOffset + relativeOffset;
  if (recordStart + 4 > bytes_.size()) return false;
  const auto recordLength = get_u32(bytes_, recordStart);
  if (recordLength < 4 || recordLength > bytes_.size() - recordStart) return false;
  return read_bank_record(bytes_.subspan(recordStart, recordLength), output);
}

}  // namespace midi
