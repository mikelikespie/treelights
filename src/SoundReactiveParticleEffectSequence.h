//
// Created by Michael Lewis on 8/19/15.
//

#ifndef SOUND_REACTIVE_PARTICLE_EFFECT_SEQUENCE_H
#define SOUND_REACTIVE_PARTICLE_EFFECT_SEQUENCE_H/**/

#include <vector>

#include "Control.h"
#include "SequenceBase.h"

#include <cmath>
#include <random>
#include <ctime>

const float BASS_START = 1.0f;
const float MID_START = 300.0f;
const float TREBLE_START = 4000.0f;
const float MAX_FREQUENCY = 30000.0f;


class SoundReactiveParticleEffectSequence : public SequenceBase<SoundReactiveParticleEffectSequence> {
public:
  struct Particle {
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

  SoundReactiveParticleEffectSequence(std::mt19937 *gen, int stripLength, const Clock &clock)
          : SequenceBase(stripLength, clock), gen(gen), _hueOffset(0) {
    _buffer1.resize((size_t) (stripLength), RGBLinear{0, 0, 0});
    _buffer2.resize((size_t) (stripLength), RGBLinear{0, 0, 0});
  }

  std::vector<RGBLinear> _buffer1;
  std::vector<RGBLinear> _buffer2;
  std::vector<Particle> _particles;

  static constexpr size_t MAX_PARTICLE_COUNT = 300;

  // Temporal smoothing was switched off in Aug 2023 (commit a3ceb21) — the
  // sound visualization looked better showing the painted buffer directly.
  // Flip this to true to bring back the original smoothing.
  static constexpr bool kSmoothPixelDecay = false;

  void decayPixels(float deltat) {
    if (!kSmoothPixelDecay) {
      _buffer2 = _buffer1;
      return;
    }

    const float multiple = expf(-50.0f * deltat);
    const float otherMultiple = 1.0f - multiple;

    int size = (int) _buffer1.size();

    for (int i = 0; i < size; i++) {
      auto &dest = _buffer2[i];
      const auto &src = _buffer1[i];

      dest.r = powf(sqrtf(dest.r) * multiple + sqrtf(src.r) * otherMultiple, 2.0f);
      dest.g = powf(sqrtf(dest.g) * multiple + sqrtf(src.g) * otherMultiple, 2.0f);
      dest.b = powf(sqrtf(dest.b) * multiple + sqrtf(src.b) * otherMultiple, 2.0f);

      dest.r = std::max(std::min(dest.r, 1.0f), 0.0f);
      dest.g = std::max(std::min(dest.g, 1.0f), 0.0f);
      dest.b = std::max(std::min(dest.b, 1.0f), 0.0f);

      if (i == 0) {
        dest = src;
      }
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

  inline void combineColor(RGBLinear *destination, const RGBLinear additionalColor) {
    destination->r = std::min(1.0f, destination->r + additionalColor.r);
    destination->g = std::min(1.0f, destination->g + additionalColor.g);
    destination->b = std::min(1.0f, destination->b + additionalColor.b);
  }

  inline void paintParticle(float position, float value, float hue, float saturation, const int radius, float age) {
    const float pixelPos = position * stripLength();
    const int closestIndex = (int) (pixelPos);


    //    const float fadeInMultiple = 1.0f - expf(-age * );

    // Leave at 1 for now. We'd like to fade in the particles but want
    // to do it at different speeds for different types
    const float fadeInMultiple = 1.0f;

    //        float decay = 1.0;
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

  inline void paintParticles(float deltat) {
    const int paintRadius = 4;

    for (auto const &p: _particles) {
      paintParticle(p.position, p.value, p.hue, p.saturation, paintRadius, p.age);
    }
  }

  void loop(Context *context) override {
    float deltat = clock().deltaf();
    _hueSliceMidMin = _hueSlicePhase.value() - _hueSliceSizeControl.value() * .5f;
    _hueSliceMidMax = _hueSlicePhase.value() + _hueSliceSizeControl.value() * .5f;


    for (auto &p: _buffer1) {
      p = RGBLinear{0, 0, 0};
    }

    float ax = -.0001;
    updateParticles(deltat, ax);
    paintParticles(deltat);
    decayPixels(deltat);

    SequenceBase::loop(context);
  }

  inline ARGB colorForPixel(int pixel, const Context &context) {
    return _buffer2[pixel].convertWithJitter(*gen);
  }

  const std::vector<Control *> &controls() override {
    return _controls;
  }

  const float inline k() const {
    return 0.1;
  }

  const float inline generation_k() const {
    return 20;
  }

  ~SoundReactiveParticleEffectSequence() override = default;

  void updateSoundData(const float *data) override {
    SequenceBase::updateSoundData(data);
    _soundData.updateBuffer(data);

    _bassMagnitude = _soundData.getCombinedFrequencyRange(BASS_START, MID_START);
    _midMagnitude = _soundData.getCombinedFrequencyRange(MID_START, TREBLE_START);
    _trebleMagnitude = _soundData.getCombinedFrequencyRange(TREBLE_START, MAX_FREQUENCY);
  }


private:
  std::mt19937 *gen;
  std::uniform_real_distribution<> distribution = std::uniform_real_distribution<>(0, 1);
  std::uniform_real_distribution<> lightnessDistribution = std::uniform_real_distribution<>(0.8, 1.2);
  std::uniform_real_distribution<> sparkLightnessDistribution = std::uniform_real_distribution<>(0.3, 1.2);

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

//  float _hueSliceTrebleMin;
//  float _hueSliceTrebleMax;

  const float _hueOffset;

private:
  SoundData _soundData;

  inline float calculateBassHue() {
    const float minhue = _hueSliceBassMin;
    const float maxhue = _hueSliceBassMax;
    std::uniform_real_distribution<float> dist(minhue, maxhue);
    float hue = dist(*gen) + _hueOffset;

    if (hue < 0) {
      hue -= floorf(hue);
    } else {
      hue = fmodf(hue, 1.0);
    }

    return hue;
  }

  inline float calculateMidHue() {
    const float minhue = _hueSliceMidMin;
    const float maxhue = _hueSliceMidMax;
    std::uniform_real_distribution<float> dist(minhue, maxhue);
    float hue = dist(*gen) + _hueOffset;

    if (hue < 0) {
      hue -= floorf(hue);
    } else {
      hue = fmodf(hue, 1.0);
    }

    return hue;
  }

  inline float calculateTrebleHue() {
//     returns yellow hue
    return 0.15f;
  }
};


#endif //TREELIGHTS_SINWAVESEQUENCE_H
