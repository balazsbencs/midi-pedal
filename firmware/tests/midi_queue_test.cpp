#include <cstdint>

#include <gtest/gtest.h>

#include "core/midi_queue.hpp"

TEST(MidiQueue, DropsNewestWhenFullAndPreservesOrder) {
  midi::MidiQueue<2> queue;
  const midi::MidiMessage first{{0x90, 1, 2}, 3};
  const midi::MidiMessage second{{0x90, 3, 4}, 3};
  const midi::MidiMessage third{{0x90, 5, 6}, 3};
  EXPECT_TRUE(queue.push(first));
  EXPECT_TRUE(queue.push(second));
  EXPECT_FALSE(queue.push(third));
  EXPECT_EQ(queue.size(), 2U);
  midi::MidiMessage actual{};
  ASSERT_TRUE(queue.pop(actual));
  EXPECT_EQ(actual.bytes, first.bytes);
  ASSERT_TRUE(queue.pop(actual));
  EXPECT_EQ(actual.bytes, second.bytes);
  EXPECT_FALSE(queue.pop(actual));
}

TEST(MidiQueue, HandlesWraparoundWithoutReordering) {
  midi::MidiQueue<2> queue;
  midi::MidiMessage message{{0, 0, 0}, 1};
  for (std::uint8_t value = 0; value < 32; ++value) {
    message.bytes[0] = value;
    ASSERT_TRUE(queue.push(message));
    midi::MidiMessage popped{};
    ASSERT_TRUE(queue.pop(popped));
    EXPECT_EQ(popped.bytes[0], value);
  }
}
