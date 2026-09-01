#include "switch_engine.hpp"

namespace midi {

bool SwitchEngine::elapsed(std::uint32_t now, std::uint32_t then, std::uint32_t duration) {
  return static_cast<std::uint32_t>(now - then) >= duration;
}

SwitchId SwitchEngine::switch_for_bit(unsigned bit) {
  return static_cast<SwitchId>(bit & 0x3u);
}

bool SwitchEngine::due(std::uint32_t now, std::uint32_t deadline) {
  return static_cast<std::int32_t>(now - deadline) >= 0;
}

bool SwitchEngine::is_chord_mask(std::uint8_t mask) {
  return mask == 0b0101u || mask == 0b1010u || mask == 0b0011u || mask == 0b1100u;
}

Chord SwitchEngine::chord_for_mask(std::uint8_t mask) {
  switch (mask) {
    case 0b0101u: return Chord::BankDown;  // A + C
    case 0b1010u: return Chord::BankUp;    // B + D
    case 0b0011u: return Chord::PageDown; // A + B
    case 0b1100u: return Chord::PageUp;   // C + D
    default: return Chord::None;
  }
}

void SwitchEngine::emit_single_presses(EventBatch& events, std::uint8_t mask, std::uint32_t now_ms) {
  for (unsigned bit = 0; bit < 4; ++bit) {
    const auto bit_mask = static_cast<std::uint8_t>(1u << bit);
    if ((mask & bit_mask) == 0) continue;
    const auto index = static_cast<std::size_t>(bit);
    const auto id = switch_for_bit(bit);
    events.push({SwitchEventKind::Press, id, Chord::None, now_ms});
    if (have_last_press_[index] && !elapsed(now_ms, last_press_at_[index], DoubleTapMs)) {
      events.push({SwitchEventKind::DoubleTap, id, Chord::None, now_ms});
    }
    have_last_press_[index] = true;
    last_press_at_[index] = now_ms;
    pressed_at_[index] = now_ms;
    long_emitted_[index] = false;
  }
}

void SwitchEngine::emit_releases(EventBatch& events, std::uint8_t mask, std::uint32_t now_ms) {
  for (unsigned bit = 0; bit < 4; ++bit) {
    const auto bit_mask = static_cast<std::uint8_t>(1u << bit);
    if ((mask & bit_mask) != 0) events.push({SwitchEventKind::Release, switch_for_bit(bit), Chord::None, now_ms});
  }
}

SwitchEngine::EventBatch SwitchEngine::update(std::uint8_t raw_mask, std::uint32_t now_ms) {
  EventBatch events;
  raw_mask = static_cast<std::uint8_t>(raw_mask & 0x0fu);

  if (!have_sample_) {
    have_sample_ = true;
    raw_mask_ = raw_mask;
    raw_changed_at_ = now_ms;
    if (raw_mask != 0) {
      candidate_ = true;
      candidate_started_at_ = now_ms;
    }
  } else if (raw_mask != raw_mask_) {
    const auto old_raw = raw_mask_;
    raw_mask_ = raw_mask;
    raw_changed_at_ = now_ms;
    if (!candidate_ && active_single_mask_ == 0 && !chord_active_ && old_raw == 0 && raw_mask != 0) {
      candidate_ = true;
      candidate_started_at_ = now_ms;
    }
  }

  if (raw_mask_ != stable_mask_ && elapsed(now_ms, raw_changed_at_, DebounceMs)) {
    stable_mask_ = raw_mask_;
  }

  if (chord_active_) {
    if (stable_mask_ != active_chord_mask_) {
      events.push({SwitchEventKind::ChordRelease, SwitchId::A, active_chord_, now_ms});
      chord_active_ = false;
      active_chord_mask_ = 0;
      active_chord_ = Chord::None;
      if (stable_mask_ != 0) {
        candidate_ = true;
        candidate_started_at_ = now_ms;
      }
    } else if (due(now_ms, next_repeat_at_)) {
      while (due(now_ms, next_repeat_at_)) {
        events.push({SwitchEventKind::ChordRepeat, SwitchId::A, active_chord_, now_ms});
        next_repeat_at_ += RepeatIntervalMs;
      }
    }
    return events;
  }

  if (candidate_) {
    if (stable_mask_ == 0 && raw_mask_ == 0 && elapsed(now_ms, candidate_started_at_, ChordWindowMs)) {
      candidate_ = false;
    } else if (elapsed(now_ms, candidate_started_at_, ChordWindowMs)) {
      candidate_ = false;
      if (is_chord_mask(stable_mask_)) {
        chord_active_ = true;
        active_chord_mask_ = stable_mask_;
        active_chord_ = chord_for_mask(stable_mask_);
        next_repeat_at_ = now_ms + RepeatDelayMs;
        events.push({SwitchEventKind::ChordPress, SwitchId::A, active_chord_, now_ms});
      } else if (stable_mask_ != 0) {
        active_single_mask_ = stable_mask_;
        emit_single_presses(events, active_single_mask_, now_ms);
      }
    }
  }

  if (active_single_mask_ != 0) {
    const auto released = static_cast<std::uint8_t>(active_single_mask_ & static_cast<std::uint8_t>(~stable_mask_));
    if (released != 0) {
      emit_releases(events, released, now_ms);
      active_single_mask_ = static_cast<std::uint8_t>(active_single_mask_ & static_cast<std::uint8_t>(~released));
    }
    for (unsigned bit = 0; bit < 4; ++bit) {
      const auto bit_mask = static_cast<std::uint8_t>(1u << bit);
      if ((active_single_mask_ & bit_mask) == 0) continue;
      const auto index = static_cast<std::size_t>(bit);
      if (!long_emitted_[index] && elapsed(now_ms, pressed_at_[index], LongPressMs)) {
        events.push({SwitchEventKind::LongPress, switch_for_bit(bit), Chord::None, now_ms});
        long_emitted_[index] = true;
      }
    }
  }

  // A new stable press after an active single is not eligible for the original
  // chord window; report it as an independent switch press.
  const auto added = static_cast<std::uint8_t>(stable_mask_ & static_cast<std::uint8_t>(~active_single_mask_));
  if (active_single_mask_ != 0 && added != 0) {
    active_single_mask_ = static_cast<std::uint8_t>(active_single_mask_ | added);
    emit_single_presses(events, added, now_ms);
  }
  return events;
}

}  // namespace midi
