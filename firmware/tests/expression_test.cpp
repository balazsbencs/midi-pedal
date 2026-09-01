#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include "core/expression.hpp"

namespace {
void feed(midi::ExpressionProcessor& processor, std::initializer_list<std::uint16_t> samples, std::uint32_t at) {
  for (const auto sample : samples) processor.sample(sample, at);
}
}

TEST(Expression, FiltersSpikeAndRateLimitsChanges) {
  midi::ExpressionProcessor processor({400, 3600});
  feed(processor, {400, 402, 4095, 401, 403}, 0);
  EXPECT_EQ(processor.view().value, 0);
  EXPECT_FALSE(processor.sample(2000, 10).has_value());
  EXPECT_TRUE(processor.sample(2000, 20).has_value());
}

TEST(Expression, AppliesOutputRangeAndInversion) {
  midi::ExpressionProcessor processor({400, 3600});
  midi::ExpressionAssignment assignment{};
  assignment.enabled = true;
  assignment.channel = 2;
  assignment.controller = 7;
  assignment.destination = midi::Destination::Trs;
  assignment.minimum = 20;
  assignment.maximum = 100;
  assignment.inverted = true;
  processor.set_assignment(assignment);
  feed(processor, {3600, 3600, 3600, 3600, 3600}, 0);
  ASSERT_TRUE(processor.sample(3600, 20).has_value());
  EXPECT_EQ(processor.view().value, 20U);
  EXPECT_EQ(processor.last_message().bytes[0], 0xb1U);
  EXPECT_EQ(processor.last_message().bytes[1], 7U);
  EXPECT_EQ(processor.last_message().bytes[2], 20U);
  for (int index = 0; index < 40; ++index) processor.sample(400, 40);
  EXPECT_EQ(processor.view().value, 100U);
}

TEST(Expression, RejectsShortOrReversedCalibrationAndDisconnects) {
  midi::ExpressionProcessor processor({400, 3600});
  EXPECT_FALSE(processor.set_calibration({3600, 400}));
  EXPECT_FALSE(processor.set_calibration({1000, 1200}));
  EXPECT_TRUE(processor.set_calibration({400, 3600}));
  EXPECT_TRUE(processor.view().available);
  feed(processor, {400, 400, 400, 400, 400}, 0);
  EXPECT_TRUE(processor.view().available);
  processor.sample(4095, 100);
  EXPECT_FALSE(processor.view().available);
  EXPECT_FALSE(processor.sample(4095, 120).has_value());
}

TEST(Expression, EmitsOnlyChangedValuesWithOneStepHysteresis) {
  midi::ExpressionProcessor processor({400, 3600});
  feed(processor, {2000, 2000, 2000, 2000, 2000}, 0);
  ASSERT_TRUE(processor.sample(2000, 20).has_value());
  EXPECT_FALSE(processor.sample(2001, 40).has_value());
}
