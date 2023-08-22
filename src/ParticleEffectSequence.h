//
// Created by Michael Lewis on 8/19/15.
//

#ifndef TREELIGHTS_PARTICLEEFFECTSEQUENCE_H
#define TREELIGHTS_PARTICLEEFFECTSEQUENCE_H

#include <vector>

#include "Control.h"
#include "SequenceBase.h"

#include <math.h>
#include <random>
#include <ctime>


static const int INITIAL_PARTICLE_BUFFER_SIZE = 200;
struct Particle {
  float position;
  float velocity;

  float hue;
  float saturation;
  float value;
  float age;
};

class ParticleEffectSequence : public SequenceBase<ParticleEffectSequence> {
public:
  ParticleEffectSequence(std::mt19937 *gen, int stripLength, const Clock &clock)
          : SequenceBase(stripLength, clock), _particles(INITIAL_PARTICLE_BUFFER_SIZE), gen(gen),
            _hueSliceMin(0), _hueSliceMax(0), _hueOffset(0) {
    _buffer1.resize((size_t) (stripLength), RGBLinear{0, 0, 0});
    _buffer2.resize((size_t) (stripLength), RGBLinear{0, 0, 0});
  }

  std::vector<RGBLinear> _buffer1;
  std::vector<RGBLinear> _buffer2;
  std::vector<Particle> _particles;

  void decayPixels(float deltat) {
    const float multiple = expf(-50.0f * deltat);
    const float otherMultiple = 1.0f - multiple;
//        float multiple = 0;

    int size = (int) _buffer1.size();
    for (int i = 0; i < size; i++) {
      auto &dest = _buffer2[i];
      const auto &src = _buffer1[i];
//
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

  void updateParticles(float deltat, float ax) {
    boolean create_pixel = distribution(*gen) >
                           expf(generation_k() * -deltat * (fabsf(ax) * 60.0f + 0.2f) * _generationAmount.value());

    const float min_position = -0.4f;
    const float max_position = 1.4f;


    if (create_pixel && _particles.size() < 100) {
      std::uniform_real_distribution<float> spawn_distribution(min_position, max_position);

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

    float delta_a = ax;

    float velocity_decay = expf(0.01f * -deltat);
    for (auto &p: _particles) {
      p.velocity *= velocity_decay;
      p.velocity += delta_a;
      p.position += p.velocity * deltat;
      p.age += deltat;
    }

    _particles.erase(std::remove_if(_particles.begin(), _particles.end(),
                                    [&](Particle p) {
                                      return p.position < min_position || p.position > max_position;
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


    const float fadeInMultiple = 1.0f - expf(-age * 30);


//        float decay = 1.0;
//        for (int i = 0; i <= radius; i++) {
    for (auto pixelIndex: {closestIndex, closestIndex + 1}) {
      if (pixelIndex < 0 || pixelIndex > stripLength()) {
        continue;
      }

      const float distance = fabsf(pixelIndex - pixelPos);
      const float portion = 1.0f - distance;
      const float adjustedValue = value * portion;

      const RGBLinear newColor = HSV{hue, 1.0f, adjustedValue * fadeInMultiple * _brightness.value()};

      auto &pixel = _buffer1[pixelIndex];

      combineColor(&pixel, newColor);
    }
////            decay *= 0.1;
//        }
  }

  inline void paintParticles(float deltat) {
    const int paintRadius = 3;

    for (auto const &p: _particles) {
      paintParticle(p.position, p.value, p.hue, p.saturation, paintRadius, p.age);
    }
  }

  virtual void loop(Context *context) {
    float deltat = clock().deltaf();
    float hueSlicePhase = std::fmod<float>(_hueSlicePhase.value() + 1.0f, 1.0f);
    _hueSliceMin = hueSlicePhase - _hueSliceSizeControl.value() * .5f;
    _hueSliceMax = hueSlicePhase + _hueSliceSizeControl.value() * .5f;


    for (auto &p: _buffer1) {
      p = RGBLinear{0, 0, 0};
    }

//        float ax = -(_ax.value() - 0.5f) * deltat * 2;
//    float ax = -0.9f * deltat;
    updateParticles(deltat, _ax.value() * deltat);
    paintParticles(deltat);
    decayPixels(deltat);

    SequenceBase::loop(context);

//        _hueSlicePhase.truncate(1.0f);

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


private:
  std::mt19937 *gen;
  std::uniform_real_distribution<> distribution = std::uniform_real_distribution<>(0, 1);
  std::uniform_real_distribution<> lightnessDistribution = std::uniform_real_distribution<>(0.8, 1.2);

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

private:
  inline float calculateHue() {
    const float minhue = _hueSliceMin;
    const float maxhue = _hueSliceMax;
    std::uniform_real_distribution<float> dist(minhue, maxhue);
    float hue = dist(*gen) + _hueOffset;

    if (hue < 0) {
      hue -= floorf(hue);
    } else {
      hue = fmodf(hue, 1.0);
    }

    return hue;
  }

  ~ParticleEffectSequence() override = default;
};


#endif //TREELIGHTS_SINWAVESEQUENCE_H
