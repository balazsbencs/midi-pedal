#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include "core/switch_engine.hpp"

namespace {
using midi::SwitchEngine;
using midi::SwitchEventKind;
using midi::SwitchId;

constexpr std::uint8_t mask(SwitchId id) { return static_cast<std::uint8_t>(1u << static_cast<unsigned>(id)); }

bool has_kind(const SwitchEngine::EventBatch& events, SwitchEventKind kind, SwitchId id) {
  for (const auto& event : events) {
    if (event.kind == kind && event.switch_id == id) return true;
  }
  return false;
}
}  // namespace

TEST(SwitchEngine, SuppressesAAndCWhenBankDownChordStabilizes) {
  SwitchEngine engine;
  engine.update(mask(SwitchId::A), 0);
  engine.update(mask(SwitchId::A) | mask(SwitchId::C), 12);
  const auto events = engine.update(mask(SwitchId::A) | mask(SwitchId::C), 35);
  ASSERT_EQ(events.size(), 1U);
  EXPECT_EQ(events[0].kind, SwitchEventKind::ChordPress);
  EXPECT_EQ(events[0].chord, midi::Chord::BankDown);
}

TEST(SwitchEngine, EmitsSinglePressAtChordDeadline) {
  SwitchEngine engine;
  engine.update(mask(SwitchId::B), 0);
  EXPECT_TRUE(engine.update(mask(SwitchId::B), 34).empty());
  const auto events = engine.update(mask(SwitchId::B), 35);
  ASSERT_EQ(events.size(), 1U);
  EXPECT_EQ(events[0].kind, SwitchEventKind::Press);
  EXPECT_EQ(events[0].switch_id, SwitchId::B);
}

TEST(SwitchEngine, EmitsReleaseLongPressAndDoubleTap) {
  SwitchEngine engine;
  engine.update(mask(SwitchId::D), 0);
  engine.update(mask(SwitchId::D), 35);
  engine.update(0, 100);
  EXPECT_TRUE(has_kind(engine.update(0, 120), SwitchEventKind::Release, SwitchId::D));

  engine.update(mask(SwitchId::D), 140);
  const auto secondPress = engine.update(mask(SwitchId::D), 175);
  EXPECT_TRUE(has_kind(secondPress, SwitchEventKind::Press, SwitchId::D));
  EXPECT_TRUE(has_kind(secondPress, SwitchEventKind::DoubleTap, SwitchId::D));

  const auto longPress = engine.update(mask(SwitchId::D), 675);
  EXPECT_TRUE(has_kind(longPress, SwitchEventKind::LongPress, SwitchId::D));
}

TEST(SwitchEngine, RepeatsHeldPageChordAndHandlesMillisWrap) {
  SwitchEngine engine;
  const std::uint32_t start = 0xfffffff0u;
  engine.update(mask(SwitchId::A) | mask(SwitchId::B), start);
  const auto pressed = engine.update(mask(SwitchId::A) | mask(SwitchId::B), start + 35u);
  ASSERT_EQ(pressed.size(), 1U);
  EXPECT_EQ(pressed[0].chord, midi::Chord::PageDown);
  EXPECT_TRUE(engine.update(mask(SwitchId::A) | mask(SwitchId::B), start + 634u).empty());
  const auto repeated = engine.update(mask(SwitchId::A) | mask(SwitchId::B), start + 635u);
  ASSERT_EQ(repeated.size(), 1U);
  EXPECT_EQ(repeated[0].kind, SwitchEventKind::ChordRepeat);
  EXPECT_EQ(repeated[0].chord, midi::Chord::PageDown);
}
