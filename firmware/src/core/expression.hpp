#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "config_types.hpp"

namespace midi {

struct Calibration {
  std::uint16_t heel{};
  std::uint16_t toe{4095};
};

struct ExpressionView {
  bool available{};
  std::uint8_t value{};
};

class ExpressionProcessor {
 public:
  explicit ExpressionProcessor(Calibration calibration);

  bool set_calibration(Calibration calibration);
  void set_assignment(ExpressionAssignment assignment);
  std::optional<MidiMessage> sample(std::uint16_t adc, std::uint32_t now_ms);

  [[nodiscard]] const ExpressionView& view() const { return view_; }
  [[nodiscard]] Calibration calibration() const { return calibration_; }
  [[nodiscard]] const ExpressionAssignment& assignment() const { return assignment_; }
  [[nodiscard]] const MidiMessage& last_message() const { return last_message_; }

 private:
  static constexpr std::uint16_t MinSpan = 410;
  static constexpr std::uint16_t AdcMax = 4095;

  static bool valid_calibration(Calibration calibration);
  std::uint16_t median() const;
  std::uint8_t map_value(std::uint16_t filtered) const;
  static bool elapsed(std::uint32_t now, std::uint32_t then, std::uint32_t duration);

  Calibration calibration_{};
  ExpressionAssignment assignment_{};
  ExpressionView view_{};
  std::array<std::uint16_t, 5> samples_{};
  std::uint8_t sample_count_{};
  std::uint8_t sample_cursor_{};
  std::uint32_t filtered_q_{};
  bool have_filtered_{};
  bool valid_{};
  bool have_emit_clock_{};
  std::uint32_t last_emit_at_{};
  bool have_last_emitted_{};
  std::uint8_t last_emitted_value_{};
  MidiMessage last_message_{};
};

}  // namespace midi
