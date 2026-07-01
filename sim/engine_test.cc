#include "sim/SimEngine.h"

#include <gtest/gtest.h>

namespace {

// Effective emitted energy of one LED, folding in the APA102 5-bit global
// brightness the way the firmware's writeColor does (caps at 31).
float emitted(const ARGB &led) {
  float global = std::min<uint8_t>(led.a, 31) / 31.0f;
  return global * (led.r + led.g + led.b) / 255.0f;
}

float totalLight(const SimEngine &engine) {
  float sum = 0;
  for (int i = 0; i < SimEngine::kTotalLedCount; i++) {
    sum += emitted(engine.leds()[i]);
  }
  return sum;
}

TEST(SimEngineTest, EverySequenceRendersLight) {
  SimEngine engine;

  float bins[SOUND_BUFFER_BIN_COUNT] = {0};
  bins[2] = 0.6f;    // bass
  bins[40] = 0.4f;   // mid
  bins[200] = 0.3f;  // treble

  for (int seq = 0; seq < engine.sequenceCount(); seq++) {
    engine.setSequenceIndex(seq);
    uint32_t base = 1000 + seq * 100000;
    for (int frame = 0; frame < 240; frame++) {
      engine.tick(base + frame * 16, frame % 3 == 0 ? bins : nullptr);
    }
    EXPECT_GT(totalLight(engine), 0.0f) << engine.sequenceName(seq);
  }
}

TEST(SimEngineTest, ControlsAffectOutput) {
  SimEngine engine;  // sequence 0: Particles; control 0 is brightness

  for (int frame = 0; frame < 120; frame++) {
    engine.tick(1000 + frame * 16, nullptr);
  }
  float withDefaults = totalLight(engine);
  EXPECT_GT(withDefaults, 0.0f);

  // Slam brightness + generation to zero: light should die out.
  for (int i = 0; i < SimEngine::kMaxControls; i++) {
    engine.setControlValue(i, 0.0f);
  }
  for (int frame = 0; frame < 600; frame++) {
    engine.tick(10000 + frame * 16, nullptr);
  }
  EXPECT_LT(totalLight(engine), withDefaults * 0.01f);
}

TEST(SimEngineTest, SwitchingSequencesIsStable) {
  SimEngine engine;
  float bins[SOUND_BUFFER_BIN_COUNT] = {0};
  bins[3] = 0.5f;

  for (int frame = 0; frame < 400; frame++) {
    engine.setSequenceIndex(frame / 50 % engine.sequenceCount());
    engine.tick(1000 + frame * 16, frame % 2 == 0 ? bins : nullptr);
  }
}

}  // namespace
