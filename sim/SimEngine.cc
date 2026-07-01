#include "sim/SimEngine.h"

#include <algorithm>
#include <cstring>

#include "ParticleEffectSequence.h"
#include "SinWaveSequence.h"
#include "SoundHistogramSequence.h"
#include "SoundReactiveParticleEffectSequence.h"

namespace {

struct SequenceSpec {
  const char *name;
  Sequence *(*make)(std::mt19937 *gen, int stripLength, const Clock &clock);
};

// Same set and order as the firmware's `sequences` table.
const SequenceSpec kSequences[] = {
    {"Particles",
     [](std::mt19937 *g, int l, const Clock &c) -> Sequence * { return new ParticleEffectSequence(g, l, c); }},
    {"Sin Wave",
     [](std::mt19937 *g, int l, const Clock &c) -> Sequence * { return new SinWaveSequence(l, c); }},
    {"Sound Histogram",
     [](std::mt19937 *g, int l, const Clock &c) -> Sequence * { return new SoundHistogramSequence(l, c); }},
    {"Sound Particles",
     [](std::mt19937 *g, int l, const Clock &c) -> Sequence * {
       return new SoundReactiveParticleEffectSequence(g, l, c);
     }},
};

const int kSequenceCount = sizeof(kSequences) / sizeof(kSequences[0]);

}  // namespace

SimEngine::SimEngine() {
  _contexts.reserve(kStripCount);
  for (int s = 0; s < kStripCount; s++) {
    _contexts.emplace_back(_leds + s * kStripLength, kStripLength, false);
  }
  _instances.assign(kSequenceCount * kStripCount, nullptr);
}

SimEngine::~SimEngine() {
  for (Sequence *sequence : _instances) {
    delete sequence;
  }
}

Sequence *SimEngine::instanceFor(int sequenceIndex, int strip) {
  Sequence *&slot = _instances[sequenceIndex * kStripCount + strip];
  if (slot == nullptr) {
    slot = kSequences[sequenceIndex].make(&_gen, kStripLength, _clock);
    slot->initialize();
  }
  return slot;
}

void SimEngine::tick(uint32_t nowMillis, const float *fft512) {
  _clock.tickWithMillis(nowMillis);

  if (fft512 != nullptr) {
    memcpy(_soundBuffer, fft512, sizeof(_soundBuffer));
    for (int strip = 0; strip < kStripCount; strip++) {
      instanceFor(_sequenceIndex, strip)->updateSoundData(_soundBuffer);
    }
  }

  for (int strip = 0; strip < kStripCount; strip++) {
    Sequence *sequence = instanceFor(_sequenceIndex, strip);

    const std::vector<Control *> &controls = sequence->controls();
    int count = std::min<int>((int) controls.size(), kMaxControls);
    for (int i = 0; i < count; i++) {
      if (_controlsTouched) {
        controls[i]->tick(_clock, _controlValues[i]);
      } else {
        controls[i]->tick(_clock);
      }
    }

    sequence->loop(&_contexts[strip]);
  }
}

int SimEngine::sequenceCount() const {
  return kSequenceCount;
}

const char *SimEngine::sequenceName(int index) const {
  return kSequences[index].name;
}

void SimEngine::setSequenceIndex(int index) {
  if (index < 0 || index >= kSequenceCount || index == _sequenceIndex) {
    return;
  }
  _sequenceIndex = index;
  // Like the firmware: refresh sound data on instances we're switching back to.
  for (int strip = 0; strip < kStripCount; strip++) {
    Sequence *existing = _instances[index * kStripCount + strip];
    if (existing != nullptr) {
      existing->updateSoundData(_soundBuffer);
    }
  }
}

int SimEngine::controlCount() {
  int count = (int) instanceFor(_sequenceIndex, 0)->controls().size();
  return std::min(count, (int) kMaxControls);
}

void SimEngine::setControlValue(int channel, float value) {
  if (channel < 0 || channel >= kMaxControls) {
    return;
  }
  _controlValues[channel] = std::max(0.0f, std::min(1.0f, value));
  _controlsTouched = true;
}
