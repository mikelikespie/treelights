#ifndef TREELIGHTS_SIM_SIMENGINE_H
#define TREELIGHTS_SIM_SIMENGINE_H

#include <cstdint>
#include <random>
#include <vector>

#include "Color.h"
#include "Context.h"
#include "Control.h"
#include "SequenceBase.h"
#include "SoundData.h"
#include "clock.h"

/// Which physical build the engine emulates.
enum class SimFixture {
  /// particles_with_sound.cc: 8 strips x 118 LEDs.
  kBars,
  /// ball_loop_2.cc: one 540-LED strip along the 30 edges (18 LEDs each) of
  /// an icosahedron.
  kBall,
};

/// Headless re-creation of the firmware main loop (particles_with_sound.cc /
/// ball_loop_2.cc): same strip geometry, sequence set, control ticking, and
/// sound plumbing, with wall time, sound bins, and DMX values injected
/// instead of read from hardware.
class SimEngine {
public:
  static constexpr int kMaxControls = 16;  // DMX_CHANNEL_COUNT on the device

  static constexpr int kBallSegmentLength = 18;
  static constexpr int kBallSegmentCount = 30;  // icosahedron edges

  explicit SimEngine(SimFixture fixture = SimFixture::kBars);
  ~SimEngine();

  SimEngine(const SimEngine &) = delete;
  SimEngine &operator=(const SimEngine &) = delete;

  SimFixture fixture() const { return _fixture; }
  int stripCount() const { return _stripCount; }
  int stripLength() const { return _stripLength; }
  int totalLedCount() const { return _stripCount * _stripLength; }

  /// Advances one frame. fft512 may be null (no fresh audio this frame);
  /// otherwise it must point at SOUND_BUFFER_BIN_COUNT floats.
  void tick(uint32_t nowMillis, const float *fft512);

  int sequenceCount() const;
  const char *sequenceName(int index) const;
  int sequenceIndex() const { return _sequenceIndex; }
  void setSequenceIndex(int index);

  /// Number of DMX-mapped controls on the current sequence.
  int controlCount();

  /// Sets a control input (0..1), like a DMX channel. Mirrors the firmware's
  /// seenNonZeroDmxValue behavior: until the first call, controls tick
  /// without input and hold their defaults.
  void setControlValue(int channel, float value);

  const ARGB *leds() const { return _leds.data(); }

private:
  Sequence *instanceFor(int sequenceIndex, int strip);

  const SimFixture _fixture;
  const int _stripCount;
  const int _stripLength;

  Clock _clock;
  std::mt19937 _gen{0};
  std::vector<ARGB> _leds;
  std::vector<Context> _contexts;
  std::vector<Sequence *> _instances;  // sequenceCount x stripCount, lazy
  SoundDataBuffer _soundBuffer = {0};
  int _sequenceIndex = 0;
  bool _controlsTouched = false;
  float _controlValues[kMaxControls] = {0};
};

#endif  // TREELIGHTS_SIM_SIMENGINE_H
