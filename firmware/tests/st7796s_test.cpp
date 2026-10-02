#include <array>
#include <vector>

#include <gtest/gtest.h>

#include "display_sdk.hpp"
#include "display/st7796s.hpp"

namespace {
void clear_frame(midi::display::St7796sDisplay& display) {
  const std::vector<std::uint16_t> pixels(480 * 320, 0);
  display.write({0, 0, 480, 320}, pixels);
}

void finish_frame(midi::display::St7796sDisplay& display) {
  display.service();
  unsigned iterations = 0;
  while (!display_sdk::cs_high) {
    ASSERT_LT(++iterations, 1000U);
    if (display_sdk::dma_active) complete_dma();
    display.service();
  }
}
}  // namespace

TEST(St7796s, CopiesCallerPixelsAndKeepsChipSelectedUntilLastBitFinishes) {
  display_sdk::reset();
  midi::display::St7796sDisplay display;
  display.initialize();
  clear_frame(display);
  std::array<std::uint16_t, 2> pixels{0x1234, 0xABCD};
  display.write({0, 0, 2, 1}, pixels);
  pixels.fill(0);
  EXPECT_FALSE(display_sdk::dma_active);
  display.service();
  EXPECT_TRUE(display_sdk::dma_active);
  EXPECT_FALSE(display_sdk::cs_high);
  finish_frame(display);
  ASSERT_EQ(display_sdk::pixel_bytes.size(), 480U * 320U * 2U);
  EXPECT_EQ(display_sdk::pixel_bytes[0], 0x12);
  EXPECT_EQ(display_sdk::pixel_bytes[1], 0x34);
  EXPECT_EQ(display_sdk::pixel_bytes[2], 0xAB);
  EXPECT_EQ(display_sdk::pixel_bytes[3], 0xCD);
  EXPECT_FALSE(display_sdk::premature_cs);
  EXPECT_EQ(display_sdk::windows, 1U);
  EXPECT_EQ(display_sdk::dma_starts, 80U);
  EXPECT_EQ(display_sdk::blocking_dma_waits, 0U);
}

TEST(St7796s, ReturnsWhileDmaOrSpiIsBusyWithoutReusingStagingBuffer) {
  display_sdk::reset();
  midi::display::St7796sDisplay display;
  display.initialize();
  clear_frame(display);
  const std::array<std::uint16_t, 1> first{0x1234}, second{0x5678};
  display.write({0, 0, 1, 1}, first);
  display.service();
  display.write({0, 0, 1, 1}, second);
  display.service();
  EXPECT_EQ(display_sdk::dma_starts, 1U);
  EXPECT_EQ(display_sdk::pixel_bytes.size(), 0U);
  complete_dma();
  display.service();  // DMA finished, but final bits are still on the wire.
  EXPECT_EQ(display_sdk::dma_starts, 1U);
  EXPECT_FALSE(display_sdk::cs_high);
  finish_frame(display);
  ASSERT_GE(display_sdk::pixel_bytes.size(), 2U);
  EXPECT_EQ(display_sdk::pixel_bytes[0], 0x12);
  EXPECT_EQ(display_sdk::pixel_bytes[1], 0x34);
  finish_frame(display);  // The update made during DMA needs another full frame.
  const auto second_frame = 480U * 320U * 2U;
  ASSERT_EQ(display_sdk::pixel_bytes.size(), second_frame * 2);
  EXPECT_EQ(display_sdk::pixel_bytes[second_frame], 0x56);
  EXPECT_EQ(display_sdk::pixel_bytes[second_frame + 1], 0x78);
  EXPECT_EQ(display_sdk::windows, 2U);
  EXPECT_FALSE(display_sdk::premature_cs);
  EXPECT_EQ(display_sdk::blocking_dma_waits, 0U);
}

TEST(St7796s, CopiesLargeRectanglesWithoutTruncatingPixels) {
  display_sdk::reset();
  midi::display::St7796sDisplay display;
  display.initialize();
  clear_frame(display);
  const std::vector<std::uint16_t> pixels(480 * 12, 0x07E0);
  display.write({0, 0, 480, 12}, pixels);
  finish_frame(display);
  ASSERT_EQ(display_sdk::pixel_bytes.size(), 480U * 320U * 2U);
  for (std::size_t index = 0; index < pixels.size(); ++index) {
    EXPECT_EQ(display_sdk::pixel_bytes[index * 2], 0x07);
    EXPECT_EQ(display_sdk::pixel_bytes[index * 2 + 1], 0xE0);
  }
  EXPECT_EQ(display_sdk::windows, 1U);
  EXPECT_FALSE(display_sdk::premature_cs);
}

TEST(St7796s, RejectsIncompleteAndOutOfBoundsRectanglesWithoutStartingIo) {
  display_sdk::reset();
  midi::display::St7796sDisplay display;
  display.initialize();
  const std::array<std::uint16_t, 1> pixels{0x1234};
  display.write({0, 0, 2, 2}, pixels);
  display.write({480, 0, 1, 1}, pixels);
  display.write({0, 320, 1, 1}, pixels);
  display.service();
  EXPECT_FALSE(display_sdk::dma_active);
  EXPECT_EQ(display_sdk::windows, 0U);
}
