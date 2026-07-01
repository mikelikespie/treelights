#include <random>

#include "Context.h"
#include "ExampleSequence.h"
#include "ParticleEffectSequence.h"
#include "SoundReactiveParticleEffectSequence.h"

#include <gtest/gtest.h>

namespace {

Clock makeClock(uint32_t deltaMs) {
  Clock clock;
  clock.tickWithMillis(1000);
  clock.tickWithMillis(1000 + deltaMs);
  return clock;
}

void tickAllControls(Sequence *sequence, const Clock &clock, float input) {
  for (Control *control : sequence->controls()) {
    control->tick(clock, input);
  }
}

bool colorsEqual(const ARGB &a, const ARGB &b) {
  return a.a == b.a && a.r == b.r && a.g == b.g && a.b == b.b;
}

TEST(ContextTest, ForwardWritesInOrder) {
  ARGB leds[4] = {};
  Context context(leds, 4, false);

  context.setColor(1, ARGB{31, 10, 20, 30});
  EXPECT_TRUE(colorsEqual(leds[1], ARGB{31, 10, 20, 30}));
}

TEST(ContextTest, ReverseMirrorsIndex) {
  ARGB leds[4] = {};
  Context context(leds, 4, true);

  context.setColor(0, ARGB{31, 10, 20, 30});
  EXPECT_TRUE(colorsEqual(leds[3], ARGB{31, 10, 20, 30}));
  context.setColor(3, ARGB{31, 1, 2, 3});
  EXPECT_TRUE(colorsEqual(leds[0], ARGB{31, 1, 2, 3}));
}

TEST(SequenceBaseTest, LoopFillsWholeStrip) {
  Clock clock = makeClock(16);
  ARGB leds[5] = {};
  Context context(leds, 5, false);
  ExampleSequence sequence(5, clock, ARGB{31, 10, 20, 30});

  sequence.loop(&context);

  for (const auto &led : leds) {
    EXPECT_TRUE(colorsEqual(led, ARGB{31, 10, 20, 30}));
  }
}

// Regression test: the particle buffer used to be *constructed* with 200
// zero-value zombie particles (blocking new spawns) instead of reserved.
TEST(ParticleEffectSequenceTest, StartsWithNoParticles) {
  std::mt19937 gen(1);
  Clock clock = makeClock(16);
  ParticleEffectSequence sequence(&gen, 10, clock);

  EXPECT_EQ(sequence._particles.size(), 0u);
}

TEST(ParticleEffectSequenceTest, ParticlesSpawnAndStayInBounds) {
  std::mt19937 gen(1);
  Clock clock = makeClock(16);
  ParticleEffectSequence sequence(&gen, 10, clock);
  tickAllControls(&sequence, clock, 0.7f);

  for (int i = 0; i < 200; i++) {
    sequence.updateParticles(0.016f, 0.02f);
  }

  EXPECT_GT(sequence._particles.size(), 0u);
  EXPECT_LE(sequence._particles.size(), 100u);
  for (const auto &p : sequence._particles) {
    EXPECT_GE(p.position, -0.4f);
    EXPECT_LE(p.position, 1.4f);
  }
}

// Regression test: painting a particle in the last pixel used to write one
// element past the end of the pixel buffer (pixelIndex == stripLength).
TEST(ParticleEffectSequenceTest, PaintClipsAtStripEnd) {
  std::mt19937 gen(1);
  Clock clock = makeClock(16);
  ParticleEffectSequence sequence(&gen, 10, clock);
  tickAllControls(&sequence, clock, 1.0f);

  // pixelPos 9.5: touches pixels 9 and 10; 10 must be clipped.
  sequence.paintParticle(0.95f, 1.0f, 0.5f, 1.0f, 3, 1.0f);
  const RGBLinear &last = sequence._buffer1[9];
  EXPECT_GT(last.r + last.g + last.b, 0.0f);

  // Entirely past the end: no pixel is painted, and no out-of-bounds write.
  sequence.paintParticle(1.05f, 1.0f, 0.5f, 1.0f, 3, 1.0f);
}

TEST(SoundReactiveParticleEffectSequenceTest, SpawnRespectsParticleCap) {
  std::mt19937 gen(1);
  Clock clock = makeClock(16);
  SoundReactiveParticleEffectSequence sequence(&gen, 10, clock);

  sequence.spawnParticles(1000, 0.5f, 0.9f, 1.0f);
  EXPECT_EQ(sequence._particles.size(),
            SoundReactiveParticleEffectSequence::MAX_PARTICLE_COUNT);
}

TEST(SoundReactiveParticleEffectSequenceTest, DecayIsPassthroughWhileDisabled) {
  std::mt19937 gen(1);
  Clock clock = makeClock(16);
  SoundReactiveParticleEffectSequence sequence(&gen, 10, clock);

  sequence._buffer1[3] = RGBLinear{0.5f, 0.25f, 0.125f};
  sequence.decayPixels(0.016f);

  EXPECT_FLOAT_EQ(sequence._buffer2[3].r, 0.5f);
  EXPECT_FLOAT_EQ(sequence._buffer2[3].g, 0.25f);
  EXPECT_FLOAT_EQ(sequence._buffer2[3].b, 0.125f);
}

TEST(SoundReactiveParticleEffectSequenceTest, PaintClipsAtStripEnd) {
  std::mt19937 gen(1);
  Clock clock = makeClock(16);
  SoundReactiveParticleEffectSequence sequence(&gen, 10, clock);
  tickAllControls(&sequence, clock, 1.0f);

  // pixelPos 9.5: closestIndex 9, paints up to index 11 unclipped.
  sequence.paintParticle(0.95f, 1.0f, 0.5f, 1.0f, 4, 1.0f);
  const RGBLinear &last = sequence._buffer1[9];
  EXPECT_GT(last.r + last.g + last.b, 0.0f);

  sequence.paintParticle(1.2f, 1.0f, 0.5f, 1.0f, 4, 1.0f);
}

}  // namespace
