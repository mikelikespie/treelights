#include "SoundData.h"

#include <cmath>

#include <gtest/gtest.h>

namespace {

class SoundDataTest : public ::testing::Test {
protected:
  void FillOnes() {
    SoundDataBuffer buf;
    for (int i = 0; i < SOUND_BUFFER_BIN_COUNT; i++) {
      buf[i] = 1.0f;
    }
    data.updateBuffer(buf);
  }

  void FillIndex() {
    SoundDataBuffer buf;
    for (int i = 0; i < SOUND_BUFFER_BIN_COUNT; i++) {
      buf[i] = (float) i;
    }
    data.updateBuffer(buf);
  }

  SoundData data;
};

TEST_F(SoundDataTest, CombinedRangeSumsCoveredBins) {
  FillOnes();
  // 1..300 Hz covers bins 0..6 (300 / 43 = 6.97) = 7 bins.
  EXPECT_FLOAT_EQ(data.getCombinedFrequencyRange(1.0f, 300.0f), 7.0f);
}

TEST_F(SoundDataTest, CombinedRangeClampsToLastBin) {
  FillOnes();
  // 30 kHz maps past bin 511 and clamps: bins 93..511 = 419 bins.
  EXPECT_FLOAT_EQ(data.getCombinedFrequencyRange(4000.0f, 30000.0f), 419.0f);
}

TEST_F(SoundDataTest, LowPixelReadsSingleBin) {
  FillIndex();
  // Pixel 0 of 118 maps to ~53-55 Hz, entirely inside bin 1.
  EXPECT_FLOAT_EQ(data.getValueForPixel(0, 118), 1.0f);
}

TEST_F(SoundDataTest, HighPixelSumsMultipleBins) {
  FillOnes();
  // The top pixel spans many bins at the log-scaled high end; the value is
  // the bin count summed (~10 bins around 4.8-5.2 kHz).
  float value = data.getValueForPixel(117, 118);
  EXPECT_GE(value, 8.0f);
  EXPECT_LE(value, 12.0f);
}

TEST_F(SoundDataTest, OutOfRangePixelIsSafe) {
  FillOnes();
  float value = data.getValueForPixel(118, 118);
  EXPECT_TRUE(std::isfinite(value));
}

}  // namespace
