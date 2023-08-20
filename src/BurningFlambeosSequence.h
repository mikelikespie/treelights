//
// Created by Michael Lewis on 8/19/15.
//

#ifndef TREELIGHTS_BurningFlambeosSequence_H
#define TREELIGHTS_BurningFlambeosSequence_H

#include <vector>

#include "Control.h"
#include "SequenceBase.h"

#include <math.h>

#include "ledmath.h"

class BurningFlambeosSequence : public SequenceBase<BurningFlambeosSequence> {


public:
  BurningFlambeosSequence(int stripLength, const Clock &clock)
          : SequenceBase(stripLength,
                         clock) {}

  virtual void loop(Context *context) {
        _hueSliceMin = -0.02;
        _hueSliceMax = 0.08;
//    _hueSliceMin = _hueSlicePhase.value() - _hueSliceSizeControl.value() * .5f;
//    _hueSliceMax = _hueSlicePhase.value() + _hueSliceSizeControl.value() * .5f;


    SequenceBase<BurningFlambeosSequence>::loop(context);

    _lightnessPhase.truncate(M_TWOPI * _wavelength.value());
    _colorPhase.truncate((M_TWOPI * _colorWavelength.value()));
    _hueSlicePhase.truncate(1.0);
  }

  inline ARGB colorForPixel(int pixel, const Context &context) {
    float sinOffsetBase = ((float)pixel - (float)stripLength() / 2.0f) * M_TWOPI / _wavelength.value();

    float sinOffsetV = sinOffsetBase + _lightnessPhase.value() / _wavelength.value();
//        float v = (sinf(sinOffsetV) + 1) * 0.5f;
    float v = sawtooth(sinOffsetV / M_TWOPI);
    v *= v;

    v *= 3;
    v -= 0.75;

    v = std::min(1.0f, v);
    v = std::max(0.0f, v);

    float sinOffsetBaseH = ((float)pixel - (float)stripLength() / 2.0f) * M_TWOPI / _colorWavelength.value();

    float sinOffsetH = sinOffsetBaseH + _colorPhase.value() / _colorWavelength.value();

    float sinOffsetAdjustedH = sinOffsetH / M_TWOPI;

    float hue = calculateHue(sinOffsetAdjustedH);


    return ((RGBLinear) (HSV{hue, 0.9f, v})).convertWithJitter(generator);
//        return ARGB { 8, 255, 0, 0 };
  }

  float calculateHue(float sinOffsetAdjustedH) const {
    float hue = (sawtooth(sinOffsetAdjustedH)) * (_hueSliceMax - _hueSliceMin) + _hueSliceMin;

//        float hue = _hueSliceMin;

    if (hue < 0) {
      hue -= floor(hue);
    } else {
      hue = fmodf(hue, 1.0);
    }
    return hue;
  }


  virtual const std::vector<Control *> &controls() {
    return _controls;
  }

private:
  SmoothAccumulatorControl _lightnessPhase = SmoothAccumulatorControl(525.0, -500.0,
                                                                      20); // This should probably be an accumulator
  SmoothAccumulatorControl _colorPhase = SmoothAccumulatorControl(-500.0, 500.0, -31);
  SmoothAccumulatorControl _hueSlicePhase = SmoothAccumulatorControl(0.0001, 0.035, 0.015);

  std::mt19937 generator = std::mt19937(0);
  SmoothLinearControl _wavelength = SmoothLinearControl (10, 100, 8.5);
  SmoothLinearControl _colorWavelength = SmoothLinearControl(20, 150, 120);
  SmoothLinearControl _centerOfWaveControl = SmoothLinearControl(-stripLength(), 0, stripLength() / -2);
  SmoothLinearControl _hueSliceSizeControl = SmoothLinearControl(0.01, 0.33, 0.15);
//  SmoothLinearControl _saturationControl = SmoothLinearControl(1, 1, 1);

  float _hueSliceMin = 0;
  float _hueSliceMax = 0;
  const std::vector<Control *> _controls = {
          &_lightnessPhase,
          &_colorPhase,
          &_wavelength,
          &_hueSlicePhase,
          &_hueSliceSizeControl,
          &_colorWavelength,
//          &_centerOfWaveControl,
//          &_saturationControl,
  };
};


#endif //TREELIGHTS_BurningFlambeosSequence_H
