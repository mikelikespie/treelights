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

  void decayPixels(float deltat) {
    const float multiple = expf(-50.0f * deltat);
    const float otherMultiple = 1.0f - multiple;

    int size = (int) _buffer1.size();

    for (int i = 0; i < size; i++) {
      auto &dest = _buffer2[i];
      const auto &src = _buffer1[i];
//
//      dest.r = powf(sqrtf(dest.r) * multiple + sqrtf(src.r) * otherMultiple, 2.0f);
//      dest.g = powf(sqrtf(dest.g) * multiple + sqrtf(src.g) * otherMultiple, 2.0f);
//      dest.b = powf(sqrtf(dest.b) * multiple + sqrtf(src.b) * otherMultiple, 2.0f);
//
//      dest.r = std::max(std::min(dest.r, 1.0f), 0.0f);
//      dest.g = std::max(std::min(dest.g, 1.0f), 0.0f);
//      dest.b = std::max(std::min(dest.b, 1.0f), 0.0f);
//
//      if (i == 0) {
      dest = src;
//      }
    }
  }

  void updateParticles(float deltat, float ax) {
//    bool create_pixel = distribution(*gen) >
//                        expf(generation_k() * -deltat * (fabsf(ax) * 60.0f + 0.2f) * _generationAmount.value());

//    int number_of_pixels_to_create = distribution
    // Similar to create pixel, and uses same distribution, but calculates nubmer of pixes to be created, which can
    // infinite, but unlikely since its a uniform distribution

    int number_of_bass_pixels_to_create = std::min(100,
                                                   (int) (distribution(*gen) * .08 * _bassMagnitude * _gain.value() +
                                                          .965));
    int number_of_mid_pixels_to_create = std::min(100, (int) (distribution(*gen) * .25 * _midMagnitude * _gain.value() +
                                                              .8));
    int number_of_treble_pixels_to_create = std::min(100, (int) (distribution(*gen) * .08 * _trebleMagnitude *
                                                                 _gain.value() + .97));

    const float min_position = -0.4f;
    const float max_position = 1.4f;

    // update velocity

    float delta_a = ax;

    float velocity_decay = expf(0.01f * -deltat);
    for (auto &p: _particles) {
      p.velocity *= velocity_decay;
      p.velocity += delta_a;
      p.position += p.velocity * deltat *
                    ((_bassMagnitude * 1.3 * _gain.value()) * (_bassMagnitude * 1.3 * _gain.value()) +
                     .025); // TODO make specific to only some particles
      p.age += deltat;

      if (p.valueDecayK > 0) {
        float value_decay = expf(-deltat * p.valueDecayK);
        p.value *= value_decay;
      }
    }

    for (int i = 0; i < number_of_bass_pixels_to_create && _particles.size() < 300; i++) {
      std::uniform_real_distribution<float> spawn_distribution(min_position, max_position);

//      float hue = calculateBassHue();
      float brightness = lightnessDistribution(*gen);
      float saturation = distribution(*gen);

      saturation = sqrtf(saturation) * 0.5 + 0.5;
      brightness *= brightness;

      _particles.emplace_back(
              Particle{(ax > 0 ? 1.0f : 0.0f) + std::normal_distribution<float>(0.0f, 0.04f)(*gen),
                       std::normal_distribution<float>(1.2f, 0.04f)(*gen),
                       .95,
                       1, brightness, 0, .8});
    }

    for (int i = 0; i < number_of_mid_pixels_to_create && _particles.size() < 300; i++) {
      std::uniform_real_distribution<float> spawn_distribution(min_position, max_position);

//      float hue = calculateBassHue();
      float brightness = lightnessDistribution(*gen);
      float saturation = distribution(*gen);

      saturation = sqrtf(saturation) * 0.5 + 0.5;
      brightness *= brightness;

      _particles.emplace_back(
              Particle{(ax > 0 ? 1.0f : 0.0f) + std::normal_distribution<float>(0.0f, 0.04f)(*gen),
                       std::normal_distribution<float>(1.2f, 0.04f)(*gen),
                       .75,
                       1, brightness, 0, 1.5});
    }
    for (int i = 0; i < number_of_treble_pixels_to_create && _particles.size() < 300; i++) {
      std::uniform_real_distribution<float> spawn_distribution(min_position, max_position);

//      float hue = calculateBassHue();
      float brightness = lightnessDistribution(*gen);
//      float saturation = distribution(*gen);

//      saturation = sqrtf(saturation) * 0.5 + 0.5;
      brightness *= brightness;

      _particles.emplace_back(
              Particle{(ax > 0 ? 1.0f : 0.0f) + std::normal_distribution<float>(0.0f, 0.04f)(*gen),
                       std::normal_distribution<float>(1.2f, 0.04f)(*gen),
                       .6,
                       1, brightness, 0, 2});
    }
//    for (int i = 0; i < number_of_mid_pixels_to_create  && _particles.size() < 300; i++) {
//      std::uniform_real_distribution<float> spawn_distribution(min_position, max_position);
//
//      float hue = calculateMidHue();
//      float brightness = lightnessDistribution(*gen);
//      float saturation = distribution(*gen);
//
//      saturation = sqrtf(saturation) * 0.5 + 0.5;
//      brightness *= brightness;
//
//      _particles.emplace_back(
//              Particle{(ax > 0 ? 1.0f : 0.0f) + std::normal_distribution<float>(0.0f, 0.08f)(*gen),
//                       std::normal_distribution<float>(1.2f, 0.04f)(*gen),
//                       hue,
//                       saturation,
//                       brightness, 0, .1});
//    }

//    for (int i = 0; i < number_of_treble_pixels_to_create  && _particles.size() < 300; i++) {
//      std::uniform_real_distribution<float> spawn_distribution(min_position, max_position);
//
//      float hue = calculateTrebleHue();
//      float brightness = sparkLightnessDistribution(*gen);
//      float saturation = distribution(*gen);
//
//      saturation = sqrtf(saturation) * 0.5 + 0.5;
//      brightness *= brightness;
//
//      _particles.emplace_back(
//              Particle{std::uniform_real_distribution<float>(0.0f, 1)(*gen),
//                      0,
//                       hue,
//                       0,
//                       brightness, 0,
//                       30});
//    }




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
      if (pixelIndex < 0 || pixelIndex > stripLength()) {
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

  virtual void loop(Context *context) {
    float deltat = clock().deltaf();
    _hueSliceMidMax = _hueSlicePhase.value() - _hueSliceSizeControl.value() * .5f;
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

  virtual const std::vector<Control *> &controls() {
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
