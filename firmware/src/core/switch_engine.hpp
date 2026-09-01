#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace midi {

enum class SwitchId : std::uint8_t { A = 0, B = 1, C = 2, D = 3 };

enum class Chord : std::uint8_t {
  None = 0,
  BankDown,
  BankUp,
  PageDown,
  PageUp,
};

enum class SwitchEventKind : std::uint8_t {
  Press,
  Release,
  LongPress,
  DoubleTap,
  ChordPress,
  ChordRepeat,
  ChordRelease,
};

struct SwitchEvent {
  SwitchEventKind kind{SwitchEventKind::Press};
  SwitchId switch_id{SwitchId::A};
  Chord chord{Chord::None};
  std::uint32_t at_ms{};
};

class SwitchEngine {
 public:
  static constexpr std::uint32_t DebounceMs = 20;
  static constexpr std::uint32_t ChordWindowMs = 35;
  static constexpr std::uint32_t DoubleTapMs = 300;
  static constexpr std::uint32_t LongPressMs = 500;
  static constexpr std::uint32_t RepeatDelayMs = 600;
  static constexpr std::uint32_t RepeatIntervalMs = 200;

  class EventBatch {
   public:
    static constexpr std::size_t Capacity = 16;

    [[nodiscard]] std::size_t size() const { return size_; }
    [[nodiscard]] bool empty() const { return size_ == 0; }
    [[nodiscard]] const SwitchEvent& operator[](std::size_t index) const { return events_[index]; }
    [[nodiscard]] const SwitchEvent* begin() const { return events_.data(); }
    [[nodiscard]] const SwitchEvent* end() const { return events_.data() + size_; }

   private:
    friend class SwitchEngine;
    void push(SwitchEvent event) {
      if (size_ < events_.size()) events_[size_++] = event;
    }

    std::array<SwitchEvent, Capacity> events_{};
    std::size_t size_{};
  };

  EventBatch update(std::uint8_t raw_mask, std::uint32_t now_ms);
  [[nodiscard]] std::uint8_t stable_mask() const { return stable_mask_; }

 private:
  static bool is_chord_mask(std::uint8_t mask);
  static Chord chord_for_mask(std::uint8_t mask);
  static bool elapsed(std::uint32_t now, std::uint32_t then, std::uint32_t duration);
  static bool due(std::uint32_t now, std::uint32_t deadline);
  static SwitchId switch_for_bit(unsigned bit);

  void emit_single_presses(EventBatch& events, std::uint8_t mask, std::uint32_t now_ms);
  void emit_releases(EventBatch& events, std::uint8_t mask, std::uint32_t now_ms);

  std::uint8_t raw_mask_{};
  std::uint8_t stable_mask_{};
  std::uint32_t raw_changed_at_{};
  bool have_sample_{};

  bool candidate_{};
  std::uint32_t candidate_started_at_{};

  bool chord_active_{};
  std::uint8_t active_chord_mask_{};
  Chord active_chord_{Chord::None};
  std::uint32_t next_repeat_at_{};

  std::uint8_t active_single_mask_{};
  std::array<std::uint32_t, 4> pressed_at_{};
  std::array<bool, 4> long_emitted_{};
  std::array<std::uint32_t, 4> last_press_at_{};
  std::array<bool, 4> have_last_press_{};
};

}  // namespace midi
