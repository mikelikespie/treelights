#ifndef TREELIGHTS_PARTICLESEQUENCEBASE_H
#define TREELIGHTS_PARTICLESEQUENCEBASE_H

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "SequenceBase.h"

/// Shared machinery for particle-style sequences: the double pixel buffer,
/// particle storage, the paint loop, temporal decay, and hue-slice helpers.
///
/// Derived classes (CRTP `T`) implement:
///   - updateParticles(float deltat, float ax)
///   - paintParticle(float position, float value, float hue, float saturation, float age)
///   - decayPixels(float deltat)  — usually forwards to decayPixelsSmoothed or
///     decayPixelsPassthrough
/// and drive each frame with runParticleFrame().
template<class T, class ParticleT>
class ParticleSequenceBase : public SequenceBase<T> {
public:
  ParticleSequenceBase(std::mt19937 *gen, int stripLength, const Clock &clock)
          : SequenceBase<T>(stripLength, clock), gen(gen) {
    _buffer1.resize((size_t) stripLength, RGBLinear{0, 0, 0});
    _buffer2.resize((size_t) stripLength, RGBLinear{0, 0, 0});
  }

  std::vector<RGBLinear> _buffer1;
  std::vector<RGBLinear> _buffer2;
  std::vector<ParticleT> _particles;

  /// Clears the paint buffer, updates/paints particles, decays into the
  /// display buffer, and renders through SequenceBase::loop.
  void runParticleFrame(Context *context, float deltat, float ax) {
    for (auto &p: _buffer1) {
      p = RGBLinear{0, 0, 0};
    }

    static_cast<T *>(this)->updateParticles(deltat, ax);
    paintParticles();
    static_cast<T *>(this)->decayPixels(deltat);

    SequenceBase<T>::loop(context);
  }

  inline void paintParticles() {
    for (auto const &p: _particles) {
      static_cast<T *>(this)->paintParticle(p.position, p.value, p.hue, p.saturation, p.age);
    }
  }

  inline void combineColor(RGBLinear *destination, const RGBLinear additionalColor) {
    destination->r = std::min(1.0f, destination->r + additionalColor.r);
    destination->g = std::min(1.0f, destination->g + additionalColor.g);
    destination->b = std::min(1.0f, destination->b + additionalColor.b);
  }

  /// Exponentially converges the display buffer toward the paint buffer,
  /// blending in sqrt space. Pixel 0 shows unsmoothed (historical quirk).
  void decayPixelsSmoothed(float deltat) {
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

  /// Shows the painted buffer directly, no temporal smoothing.
  void decayPixelsPassthrough() {
    _buffer2 = _buffer1;
  }

  inline ARGB colorForPixel(int pixel, const Context &context) {
    return _buffer2[pixel].convertWithJitter(*gen);
  }

  /// Random hue drawn from [minHue, maxHue] shifted by offset, wrapped to [0, 1).
  inline float randomHueInSlice(float minHue, float maxHue, float hueOffset) {
    std::uniform_real_distribution<float> dist(minHue, maxHue);
    float hue = dist(*gen) + hueOffset;

    if (hue < 0) {
      hue -= floorf(hue);
    } else {
      hue = fmodf(hue, 1.0);
    }

    return hue;
  }

protected:
  std::mt19937 *gen;
  std::uniform_real_distribution<> distribution = std::uniform_real_distribution<>(0, 1);
  std::uniform_real_distribution<> lightnessDistribution = std::uniform_real_distribution<>(0.8, 1.2);
};

#endif //TREELIGHTS_PARTICLESEQUENCEBASE_H
