#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace midi {

struct MidiMessage {
  std::array<std::uint8_t, 3> bytes{};
  std::uint8_t length{};
};

template <std::size_t Capacity>
struct AsciiString {
  std::array<char, Capacity + 1> data{};
  std::uint8_t length{};
};

struct PositionView {
  AsciiString<12> label{};
  std::uint16_t accentRgb565{};
};

enum class MessageKind : std::uint8_t { Pc, Cc, Relay, Navigation };
enum class Destination : std::uint8_t { Trs, Usb, Both };
enum class Trigger : std::uint8_t { Press, Release, LongPress, DoubleTap };
enum class PositionFilter : std::uint8_t { Position1, Position2, Both };

struct Message {
  MessageKind kind{MessageKind::Pc};
  Destination destination{Destination::Both};
  std::uint8_t channel{};
  std::uint8_t data1{};
  std::uint8_t data2{};
  std::uint8_t contact{};
  std::uint8_t operation{};
  std::uint8_t target{};
};

struct MessageSlot {
  std::uint32_t id{};
  Trigger trigger{Trigger::Press};
  PositionFilter position{PositionFilter::Both};
  Message message{};
};

struct Preset {
  std::uint32_t id{};
  PositionView position1{};
  PositionView position2{};
  std::uint8_t toggleOn{};
  std::array<MessageSlot, 8> slots{};
  std::uint8_t slotCount{};
};

struct Page {
  std::uint32_t id{};
  std::array<Preset, 4> presets{};
};

struct ExpressionAssignment {
  bool enabled{};
  AsciiString<12> label{};
  std::uint8_t channel{};
  std::uint8_t controller{};
  Destination destination{Destination::Both};
  std::uint8_t minimum{};
  std::uint8_t maximum{};
  bool inverted{};
};

struct BankConfig {
  std::uint32_t id{};
  AsciiString<20> name{};
  std::array<Page, 4> pages{};
  ExpressionAssignment expression{};
};

}  // namespace midi
