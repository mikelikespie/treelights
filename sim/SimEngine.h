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

/// Headless re-creation of the firmware main loop (particles_with_sound.cc):
/// same strip geometry, sequence set, control ticking, and sound plumbing,
/// with wall time, sound bins, and DMX values injected instead of read from
/// hardware.
class SimEngine {
public:
  static constexpr int kStripCount = 8;
  static constexpr int kStripLength = 118;
  static constexpr int kTotalLedCount = kStripCount * kStripLength;
  static constexpr int kMaxControls = 16;  // DMX_CHANNEL_COUNT on the device

  SimEngine();
  ~SimEngine();

  SimEngine(const SimEngine &) = delete;
  SimEngine &operator=(const SimEngine &) = delete;

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

  const ARGB *leds() const { return _leds; }

private:
  Sequence *instanceFor(int sequenceIndex, int strip);

  Clock _clock;
  std::mt19937 _gen{0};
  ARGB _leds[kTotalLedCount] = {};
  std::vector<Context> _contexts;
  std::vector<Sequence *> _instances;  // sequenceCount x stripCount, lazy
  SoundDataBuffer _soundBuffer = {0};
  int _sequenceIndex = 0;
  bool _controlsTouched = false;
  float _controlValues[kMaxControls] = {0};
};

#endif  // TREELIGHTS_SIM_SIMENGINE_H
