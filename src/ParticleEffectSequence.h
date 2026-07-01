//
// Created by Michael Lewis on 8/19/15.
//

#ifndef TREELIGHTS_PARTICLEEFFECTSEQUENCE_H
#define TREELIGHTS_PARTICLEEFFECTSEQUENCE_H

#include <math.h>
#include <random>
#include <vector>

#include "Control.h"
#include "ParticleSequenceBase.h"


static const int INITIAL_PARTICLE_BUFFER_SIZE = 200;
struct Particle {
  float position;
  float velocity;

  float hue;
  float saturation;
  float value;
  float age;
};

class ParticleEffectSequence : public ParticleSequenceBase<ParticleEffectSequence, Particle> {
public:
  ParticleEffectSequence(std::mt19937 *gen, int stripLength, const Clock &clock)
          : ParticleSequenceBase(gen, stripLength, clock),
            _hueSliceMin(0), _hueSliceMax(0), _hueOffset(0) {
    _particles.reserve(INITIAL_PARTICLE_BUFFER_SIZE);
  }

  void updateParticles(float deltat, float ax) {
    bool create_pixel = distribution(*gen) >
                           expf(generation_k() * -deltat * (fabsf(ax) * 60.0f + 0.2f) * _generationAmount.value());

    const float min_position = -0.4f;
    const float max_position = 1.4f;


    if (create_pixel && _particles.size() < 100) {
      float hue = calculateHue();
      float brightness = lightnessDistribution(*gen);
      float saturation = distribution(*gen);

      saturation = sqrtf(saturation) * 0.5 + 0.5;
      brightness *= brightness;

      _particles.emplace_back(
              Particle{(ax > 0 ? 1.0f : 0.0f) + std::normal_distribution<float>(0.0f, 0.04f)(*gen),
                       std::normal_distribution<float>(1.2f, 0.04f)(*gen),
                       hue,
                       saturation, brightness, 0});
    }

    // update velocity

    float velocity_decay = expf(0.01f * -deltat);
    for (auto &p: _particles) {
      p.velocity *= velocity_decay;
      p.velocity += ax;
      p.position += p.velocity * deltat;
      p.age += deltat;
    }

    _particles.erase(std::remove_if(_particles.begin(), _particles.end(),
                                    [&](Particle p) {
                                      return p.position < min_position || p.position > max_position;
                                    }), _particles.end());
  }

  inline void paintParticle(float position, float value, float hue, float saturation, float age) {
    const float pixelPos = position * stripLength();
    const int closestIndex = (int) (pixelPos);

    const float fadeInMultiple = 1.0f - expf(-age * 30);

    for (auto pixelIndex: {closestIndex, closestIndex + 1}) {
      if (pixelIndex < 0 || pixelIndex >= stripLength()) {
        continue;
      }

      const float distance = fabsf(pixelIndex - pixelPos);
      const float portion = 1.0f - distance;
      const float adjustedValue = value * portion;

      const RGBLinear newColor = HSV{hue, 1.0f, adjustedValue * fadeInMultiple * _brightness.value()};

      auto &pixel = _buffer1[pixelIndex];

      combineColor(&pixel, newColor);
    }
  }

  void decayPixels(float deltat) {
    decayPixelsSmoothed(deltat);
  }

  void loop(Context *context) override {
    float deltat = clock().deltaf();
    float hueSlicePhase = fmodf(_hueSlicePhase.value() + 1.0f, 1.0f);
    _hueSliceMin = hueSlicePhase - _hueSliceSizeControl.value() * .5f;
    _hueSliceMax = hueSlicePhase + _hueSliceSizeControl.value() * .5f;

//        float ax = -(_ax.value() - 0.5f) * deltat * 2;
//    float ax = -0.9f * deltat;
    runParticleFrame(context, deltat, _ax.value() * deltat);
  }

  const std::vector<Control *> &controls() override {
    return _controls;
  }

  const float inline generation_k() const {
    return 20;
  }


private:
  SmoothLinearControl _brightness = SmoothLinearControl(0, 1, 2);
  SmoothLinearControl _ax = SmoothLinearControl (0, -1, -.9); // This should probably be an accumulator
  SmoothLinearControl _hueSlicePhase = SmoothLinearControl(0, 1, .0);
  SmoothLinearControl _hueSliceSizeControl = SmoothLinearControl(0, 1, .1);
  SmoothLinearControl _generationAmount = SmoothLinearControl(0, 1, 0.8);

  const std::vector<Control *> _controls = {
          &_brightness,
          &_generationAmount,
//          &_ax,
          &_hueSlicePhase,
          &_hueSliceSizeControl,
  };

  float _hueSliceMin{};
  float _hueSliceMax{};
  const float _hueOffset;

  inline float calculateHue() {
    return randomHueInSlice(_hueSliceMin, _hueSliceMax, _hueOffset);
  }
};


#endif //TREELIGHTS_PARTICLEEFFECTSEQUENCE_H
