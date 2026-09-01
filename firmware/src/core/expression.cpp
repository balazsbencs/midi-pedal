#include "expression.hpp"

#include <algorithm>

namespace midi {

namespace {
ExpressionAssignment default_assignment() {
  ExpressionAssignment assignment{};
  assignment.enabled = true;
  assignment.label.length = 4;
  assignment.label.data = {'E', 'X', 'P', 'R', '\0'};
  assignment.channel = 1;
  assignment.controller = 11;
  assignment.destination = Destination::Both;
  assignment.minimum = 0;
  assignment.maximum = 127;
  return assignment;
}
}  // namespace

ExpressionProcessor::ExpressionProcessor(Calibration calibration)
    : calibration_(calibration), assignment_(default_assignment()), valid_(valid_calibration(calibration)) {
  view_.available = valid_;
}

bool ExpressionProcessor::valid_calibration(Calibration calibration) {
  return calibration.toe > calibration.heel && calibration.toe - calibration.heel >= MinSpan && calibration.toe <= AdcMax;
}

bool ExpressionProcessor::set_calibration(Calibration calibration) {
  calibration_ = calibration;
  valid_ = valid_calibration(calibration);
  sample_count_ = 0;
  sample_cursor_ = 0;
  have_filtered_ = false;
  have_last_emitted_ = false;
  view_ = {};
  view_.available = valid_;
  return valid_;
}

void ExpressionProcessor::set_assignment(ExpressionAssignment assignment) {
  assignment_ = assignment;
  have_last_emitted_ = false;
  view_ = {};
  view_.available = valid_ && assignment_.enabled;
}

bool ExpressionProcessor::elapsed(std::uint32_t now, std::uint32_t then, std::uint32_t duration) {
  return static_cast<std::uint32_t>(now - then) >= duration;
}

std::uint16_t ExpressionProcessor::median() const {
  std::array<std::uint16_t, 5> sorted{};
  const auto count = static_cast<std::size_t>(sample_count_);
  for (std::size_t index = 0; index < count; ++index) sorted[index] = samples_[index];
  std::sort(sorted.begin(), sorted.begin() + count);
  return count == 0 ? 0 : sorted[count / 2];
}

std::uint8_t ExpressionProcessor::map_value(std::uint16_t filtered) const {
  const auto clamped = std::clamp<std::uint16_t>(filtered, calibration_.heel, calibration_.toe);
  const auto span = static_cast<std::uint32_t>(calibration_.toe - calibration_.heel);
  std::uint32_t normalized = (static_cast<std::uint32_t>(clamped - calibration_.heel) * 127u + span / 2u) / span;
  if (assignment_.inverted) normalized = 127u - normalized;
  const auto outputSpan = static_cast<std::uint32_t>(assignment_.maximum - assignment_.minimum);
  return static_cast<std::uint8_t>(assignment_.minimum + (normalized * outputSpan + 63u) / 127u);
}

std::optional<MidiMessage> ExpressionProcessor::sample(std::uint16_t adc, std::uint32_t now_ms) {
  if (!valid_ || !assignment_.enabled) {
    view_ = {};
    return std::nullopt;
  }

  if (adc == 0 || adc == AdcMax) {
    view_.available = false;
    view_.value = 0;
    have_last_emitted_ = false;
    return std::nullopt;
  }

  view_.available = true;
  samples_[sample_cursor_] = std::min(adc, AdcMax);
  sample_cursor_ = static_cast<std::uint8_t>((sample_cursor_ + 1u) % samples_.size());
  if (sample_count_ < samples_.size()) ++sample_count_;
  const auto med = median();
  const auto target_q = static_cast<std::uint32_t>(med) << 8u;
  if (!have_filtered_) {
    filtered_q_ = target_q;
    have_filtered_ = true;
  } else {
    filtered_q_ = static_cast<std::uint32_t>(static_cast<std::int64_t>(filtered_q_) +
      (static_cast<std::int64_t>(target_q) - static_cast<std::int64_t>(filtered_q_)) / 5);
  }
  const auto filtered = static_cast<std::uint16_t>((filtered_q_ + 128u) >> 8u);
  const auto mapped = map_value(filtered);
  view_.value = mapped;

  if (!have_emit_clock_) {
    have_emit_clock_ = true;
    last_emit_at_ = now_ms;
    return std::nullopt;
  }
  if (!elapsed(now_ms, last_emit_at_, 20)) return std::nullopt;
  if (have_last_emitted_ && mapped == last_emitted_value_) return std::nullopt;

  MidiMessage message{};
  message.length = 3;
  message.bytes = {static_cast<std::uint8_t>(0xb0u | (assignment_.channel - 1u)), assignment_.controller, mapped};
  last_message_ = message;
  last_emitted_value_ = mapped;
  have_last_emitted_ = true;
  last_emit_at_ = now_ms;
  return message;
}

}  // namespace midi
