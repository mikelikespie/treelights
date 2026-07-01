//
// Created by Michael Lewis on 8/19/15.
//

#ifndef SOUND_REACTIVE_PARTICLE_EFFECT_SEQUENCE_H
#define SOUND_REACTIVE_PARTICLE_EFFECT_SEQUENCE_H

#include <cmath>
#include <random>
#include <vector>

#include "Control.h"
#include "ParticleSequenceBase.h"
#include "SoundData.h"

const float BASS_START = 1.0f;
const float MID_START = 300.0f;
const float TREBLE_START = 4000.0f;
const float MAX_FREQUENCY = 30000.0f;

struct SoundReactiveParticle {
  float position;
  float velocity;

  float hue;
  float saturation;
  float value;
  float age;

  /// This is a value between 0 and 1 that represents how much the particle has decayed.
  /// for stuff like sparks, we want it to be very low. For stuff like fire, we want it to be high
  float valueDecayK;
};

class SoundReactiveParticleEffectSequence
        : public ParticleSequenceBase<SoundReactiveParticleEffectSequence, SoundReactiveParticle> {
public:
  using Particle = SoundReactiveParticle;

  static constexpr size_t MAX_PARTICLE_COUNT = 300;

  // Temporal smoothing was switched off in Aug 2023 (commit a3ceb21) — the
  // sound visualization looked better showing the painted buffer directly.
  // Flip this to true to bring back the original smoothing.
  static constexpr bool kSmoothPixelDecay = false;

  SoundReactiveParticleEffectSequence(std::mt19937 *gen, int stripLength, const Clock &clock)
          : ParticleSequenceBase(gen, stripLength, clock), _hueOffset(0) {
  }

  void decayPixels(float deltat) {
    if (kSmoothPixelDecay) {
      decayPixelsSmoothed(deltat);
    } else {
      decayPixelsPassthrough();
    }
  }

  /// Spawns up to `count` particles at the strip end determined by the sign
  /// of `ax`, capped at MAX_PARTICLE_COUNT total.
  void spawnParticles(int count, float ax, float hue, float valueDecayK) {
    for (int i = 0; i < count && _particles.size() < MAX_PARTICLE_COUNT; i++) {
      float brightness = lightnessDistribution(*gen);
      brightness *= brightness;

      _particles.emplace_back(
              Particle{(ax > 0 ? 1.0f : 0.0f) + std::normal_distribution<float>(0.0f, 0.04f)(*gen),
                       std::normal_distribution<float>(1.2f, 0.04f)(*gen),
                       hue,
                       1, brightness, 0, valueDecayK});
    }
  }

  void updateParticles(float deltat, float ax) {
    // Number of pixels to create per band. The random draw makes the
    // fractional part of the spawn rate act as a probability.
    int number_of_bass_pixels_to_create = std::min(100,
                                                   (int) (distribution(*gen) * .08 * _bassMagnitude * _gain.value() +
                                                          .965));
    int number_of_mid_pixels_to_create = std::min(100, (int) (distribution(*gen) * .25 * _midMagnitude * _gain.value() +
                                                              .8));
    int number_of_treble_pixels_to_create = std::min(100, (int) (distribution(*gen) * .08 * _trebleMagnitude *
                                                                 _gain.value() + .97));

    const float min_position = -0.4f;
    const float max_position = 1.4f;

    float velocity_decay = expf(0.01f * -deltat);
    const float bass_speed = _bassMagnitude * 1.3f * _gain.value();
    const float position_scale = bass_speed * bass_speed + .025f;
    for (auto &p: _particles) {
      p.velocity *= velocity_decay;
      p.velocity += ax;
      p.position += p.velocity * deltat * position_scale; // TODO make specific to only some particles
      p.age += deltat;

      if (p.valueDecayK > 0) {
        float value_decay = expf(-deltat * p.valueDecayK);
        p.value *= value_decay;
      }
    }

    spawnParticles(number_of_bass_pixels_to_create, ax, /*hue=*/.95f, /*valueDecayK=*/.8f);
    spawnParticles(number_of_mid_pixels_to_create, ax, /*hue=*/.75f, /*valueDecayK=*/1.5f);
    spawnParticles(number_of_treble_pixels_to_create, ax, /*hue=*/.6f, /*valueDecayK=*/2.0f);

    _particles.erase(std::remove_if(_particles.begin(), _particles.end(),
                                    [&](Particle p) {
                                      return p.position < min_position || p.position > max_position || p.value < .004f;
                                    }), _particles.end());
  }

  inline void paintParticle(float position, float value, float hue, float saturation, float age) {
    const float pixelPos = position * stripLength();
    const int closestIndex = (int) (pixelPos);

    //    const float fadeInMultiple = 1.0f - expf(-age * );

    // Leave at 1 for now. We'd like to fade in the particles but want
    // to do it at different speeds for different types
    const float fadeInMultiple = 1.0f;

    for (auto pixelIndex: {closestIndex - 1, closestIndex, closestIndex + 1, closestIndex + 2}) {
      if (pixelIndex < 0 || pixelIndex >= stripLength()) {
        continue;
      }

      const float distance = fabsf(pixelIndex - pixelPos);
      const float portion = std::max<float>(0, std::min<float>(1.0f, (2.0f - distance) / 2));
      const float adjustedValue = value * portion;

      const RGBLinear newColor = HSV{hue, saturation, adjustedValue * fadeInMultiple * _brightness.value()};

      auto &pixel = _buffer1[pixelIndex];

      combineColor(&pixel, newColor);
    }
  }

  void loop(Context *context) override {
    float deltat = clock().deltaf();
    _hueSliceMidMin = _hueSlicePhase.value() - _hueSliceSizeControl.value() * .5f;
    _hueSliceMidMax = _hueSlicePhase.value() + _hueSliceSizeControl.value() * .5f;

    float ax = -.0001;
    runParticleFrame(context, deltat, ax);
  }

  const std::vector<Control *> &controls() override {
    return _controls;
  }

  void updateSoundData(const float *data) override {
    SequenceBase::updateSoundData(data);
    _soundData.updateBuffer(data);

    _bassMagnitude = _soundData.getCombinedFrequencyRange(BASS_START, MID_START);
    _midMagnitude = _soundData.getCombinedFrequencyRange(MID_START, TREBLE_START);
    _trebleMagnitude = _soundData.getCombinedFrequencyRange(TREBLE_START, MAX_FREQUENCY);
  }


private:
  float _bassMagnitude = 0;
  float _midMagnitude = 0;
  float _trebleMagnitude = 0;

  SmoothLinearControl _brightness = SmoothLinearControl(0.0, 2.0, 1.0);
  SmoothLinearControl _gain = SmoothLinearControl(0.0, 2.0, 1.0);
  IdentityValueControl _ax = IdentityValueControl(.5); // This should probably be an accumulator
  IdentityValueControl _hueSlicePhase = IdentityValueControl(.75);
  IdentityValueControl _hueSliceSizeControl = IdentityValueControl(.3);
  IdentityValueControl _generationAmount = IdentityValueControl(0.4);

  const std::vector<Control *> _controls = {
          &_brightness,
          &_gain,
          &_ax,
          &_hueSlicePhase,
          &_hueSliceSizeControl,
          &_generationAmount,
  };

  float _hueSliceBassMin{};
  float _hueSliceBassMax{};

  float _hueSliceMidMin{};
  float _hueSliceMidMax{};

  const float _hueOffset;

  SoundData _soundData;

  inline float calculateBassHue() {
    return randomHueInSlice(_hueSliceBassMin, _hueSliceBassMax, _hueOffset);
  }

  inline float calculateMidHue() {
    return randomHueInSlice(_hueSliceMidMin, _hueSliceMidMax, _hueOffset);
  }

  inline float calculateTrebleHue() {
//     returns yellow hue
    return 0.15f;
  }
};


#endif //SOUND_REACTIVE_PARTICLE_EFFECT_SEQUENCE_H
