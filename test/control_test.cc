#include "Control.h"

#include <gtest/gtest.h>

namespace {

// A clock whose deltaf() is deltaMs/1000.
Clock makeClock(uint32_t deltaMs) {
  Clock clock;
  clock.tickWithMillis(1000);
  clock.tickWithMillis(1000 + deltaMs);
  return clock;
}

TEST(ClockTest, FirstTickHasZeroDelta) {
  Clock clock;
  clock.tickWithMillis(5000);
  EXPECT_EQ(clock.delta(), 0u);
  EXPECT_TRUE(clock.is_first_frame());
}

TEST(ClockTest, ComputesDelta) {
  Clock clock = makeClock(16);
  EXPECT_EQ(clock.delta(), 16u);
  EXPECT_FLOAT_EQ(clock.deltaf(), 0.016f);
}

TEST(LinearlyInterpolatedValueControlTest, MapsInputOntoRange) {
  Clock clock = makeClock(16);
  LinearlyInterpolatedValueControl<float> control(2.0f, 10.0f);

  control.tick(clock, 0.0f);
  EXPECT_FLOAT_EQ(control.value(), 2.0f);
  control.tick(clock, 0.5f);
  EXPECT_FLOAT_EQ(control.value(), 6.0f);
  control.tick(clock, 1.0f);
  EXPECT_FLOAT_EQ(control.value(), 10.0f);
}

TEST(LinearlyInterpolatedValueControlTest, IntValuesTruncate) {
  Clock clock = makeClock(16);
  LinearlyInterpolatedValueControl<int> control(0, 3, 0);

  control.tick(clock, 0.34f);
  EXPECT_EQ(control.value(), 1);
  control.tick(clock, 1.0f);
  EXPECT_EQ(control.value(), 3);
}

TEST(IdentityValueControlTest, PassesInputThrough) {
  Clock clock = makeClock(16);
  IdentityValueControl control(0.5f);

  control.tick(clock, 0.25f);
  EXPECT_FLOAT_EQ(control.value(), 0.25f);
}

TEST(IdentityValueControlTest, HoldsDefaultWithoutInput) {
  Clock clock = makeClock(16);
  IdentityValueControl control(0.5f);

  control.tick(clock);
  EXPECT_FLOAT_EQ(control.value(), 0.5f);
}

TEST(BufferedControlTest, FirstTickSnapsThenConverges) {
  Clock clock = makeClock(16);
  SmoothLinearControl control(0.0f, 10.0f, 0.0f);

  control.tick(clock, 0.5f);
  EXPECT_FLOAT_EQ(control.value(), 5.0f);

  // Converges 10% toward the new target each tick.
  control.tick(clock, 1.0f);
  EXPECT_FLOAT_EQ(control.value(), 5.5f);
}

// Regression test: before any DMX input arrives the control must report the
// configured default, not 0 (which rendered everything black pre-DMX).
TEST(BufferedControlTest, ReportsWrappedDefaultBeforeAnyInput) {
  Clock clock = makeClock(16);
  SmoothLinearControl control(0.0f, 2.0f, 1.5f);

  control.tick(clock);
  EXPECT_FLOAT_EQ(control.value(), 1.5f);
}

TEST(AccumulatorControlTest, AccumulatesValueOverTime) {
  Clock clock = makeClock(500);  // deltaf = 0.5s
  SmoothAccumulatorControl control(0.0f, 2.0f, 0.0f);

  control.tick(clock, 0.5f);  // wrapped value = 1.0; accumulates 1.0 * 0.5
  EXPECT_FLOAT_EQ(control.value(), 0.5f);
  control.tick(clock, 0.5f);
  EXPECT_FLOAT_EQ(control.value(), 1.0f);
}

TEST(AccumulatorControlTest, TruncateWrapsAccumulator) {
  Clock clock = makeClock(500);
  SmoothAccumulatorControl control(0.0f, 2.0f, 0.0f);

  control.tick(clock, 0.5f);
  control.tick(clock, 0.5f);  // accumulated 1.0
  control.truncate(0.4f);     // 1.0 mod 0.4 = 0.2

  control.tick(clock, 0.5f);  // 0.2 + 0.5
  EXPECT_FLOAT_EQ(control.value(), 0.7f);
}

}  // namespace
