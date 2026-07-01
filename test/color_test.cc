#include "Color.h"
#include "ledmath.h"

#include <gtest/gtest.h>

// Note: HSV -> RGBLinear passes through RGBLog, which squares each channel
// (the "THIS IS WRONG" gamma hack). Tests below lock in that behavior.

TEST(HsvTest, PrimaryColors) {
  RGBLinear red = HSV{0.0f, 1.0f, 1.0f};
  EXPECT_FLOAT_EQ(red.r, 1.0f);
  EXPECT_FLOAT_EQ(red.g, 0.0f);
  EXPECT_FLOAT_EQ(red.b, 0.0f);

  RGBLinear green = HSV{1.0f / 3.0f, 1.0f, 1.0f};
  EXPECT_FLOAT_EQ(green.g, 1.0f);
  EXPECT_FLOAT_EQ(green.b, 0.0f);

  RGBLinear blue = HSV{2.0f / 3.0f, 1.0f, 1.0f};
  EXPECT_FLOAT_EQ(blue.b, 1.0f);
  EXPECT_FLOAT_EQ(blue.g, 0.0f);
}

TEST(HsvTest, ZeroSaturationIsGrayWithGammaSquare) {
  RGBLinear gray = HSV{0.5f, 0.0f, 0.6f};
  EXPECT_FLOAT_EQ(gray.r, 0.36f);
  EXPECT_FLOAT_EQ(gray.g, 0.36f);
  EXPECT_FLOAT_EQ(gray.b, 0.36f);
}

TEST(HsvTest, ValueZeroIsBlack) {
  RGBLinear black = HSV{0.3f, 1.0f, 0.0f};
  EXPECT_FLOAT_EQ(black.r + black.g + black.b, 0.0f);
}

TEST(HsvTest, HueOneEqualsHueZero) {
  RGBLinear h0 = HSV{0.0f, 1.0f, 0.8f};
  RGBLinear h1 = HSV{1.0f, 1.0f, 0.8f};
  EXPECT_FLOAT_EQ(h0.r, h1.r);
  EXPECT_FLOAT_EQ(h0.g, h1.g);
  EXPECT_FLOAT_EQ(h0.b, h1.b);
}

TEST(AdjustLinearFloatColorTest, ZeroIsFullyOff) {
  ARGB out = RGBLinear{0.0f, 0.0f, 0.0f};
  EXPECT_EQ(out.a, 0);
  EXPECT_EQ(out.r, 0);
  EXPECT_EQ(out.g, 0);
  EXPECT_EQ(out.b, 0);
}

// The 5-bit APA102 global-brightness trick: (a/31) * (r/255) should
// reproduce the requested linear value regardless of how the scaling split
// brightness between the two fields.
TEST(AdjustLinearFloatColorTest, BrightnessRoundTrip) {
  for (float r : {0.02f, 0.1f, 0.3f, 0.7f, 1.0f}) {
    ARGB out = RGBLinear{r, r / 2.0f, 0.0f};
    float effectiveR = (out.a / 31.0f) * (out.r / 255.0f);
    float effectiveG = (out.a / 31.0f) * (out.g / 255.0f);
    EXPECT_NEAR(effectiveR, r, 0.01f) << "r=" << r;
    EXPECT_NEAR(effectiveG, r / 2.0f, 0.01f) << "r=" << r;
  }
}

TEST(ConvertTo8bitWithJitterTest, BrightValuesRoundWithoutDither) {
  std::mt19937 rnd(42);
  EXPECT_EQ(convertTo8bitWithJitter(1.0f, &rnd), 255);
  EXPECT_EQ(convertTo8bitWithJitter(0.5f, &rnd), 128);
  // Values above 1.0 clamp instead of wrapping.
  EXPECT_EQ(convertTo8bitWithJitter(2.0f, &rnd), 255);
}

TEST(ConvertTo8bitWithJitterTest, ZeroStaysZero) {
  std::mt19937 rnd(42);
  EXPECT_EQ(convertTo8bitWithJitter(0.0f, &rnd), 0);
}

// Below the dither threshold the output should average out to the input.
TEST(ConvertTo8bitWithJitterTest, DitherPreservesMeanBrightness) {
  std::mt19937 rnd(42);
  const float input = 3.4f / 255.0f;

  double sum = 0;
  const int samples = 20000;
  for (int i = 0; i < samples; i++) {
    sum += convertTo8bitWithJitter(input, &rnd);
  }

  EXPECT_NEAR(sum / samples, 3.4, 0.1);
}

TEST(SawtoothTest, IsTriangleWaveWithPeriodOne) {
  EXPECT_FLOAT_EQ(sawtooth(0.0f), 0.0f);
  EXPECT_FLOAT_EQ(sawtooth(0.25f), 0.5f);
  EXPECT_FLOAT_EQ(sawtooth(0.5f), 1.0f);
  EXPECT_FLOAT_EQ(sawtooth(0.75f), 0.5f);
  EXPECT_FLOAT_EQ(sawtooth(1.0f), 0.0f);
  EXPECT_FLOAT_EQ(sawtooth(1.25f), 0.5f);
}
