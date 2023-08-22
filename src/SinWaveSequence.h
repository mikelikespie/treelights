//
// Created by Michael Lewis on 8/19/15.
//

#ifndef TREELIGHTS_SINWAVESEQUENCE_H
#define TREELIGHTS_SINWAVESEQUENCE_H

#include <vector>

#include "Control.h"
#include "SequenceBase.h"
#include "ledmath.h"

#define _USE_MATH_DEFINES

#include <cmath>


class SinWaveSequence : public SequenceBase<SinWaveSequence> {


public:
  SinWaveSequence(int stripLength, const Clock &clock, float brightness = 1.0f)
          : SequenceBase(stripLength,
                         clock) {}

  virtual void loop(Context *context) {

    _hueSliceMin = _hueSlicePhase.value() - _hueSliceSizeControl.value() * .5f;
    _hueSliceMax = _hueSlicePhase.value() + _hueSliceSizeControl.value() * .5f;

    SequenceBase<SinWaveSequence>::loop(context);

    _lightnessPhase.truncate(M_TWOPI * _wavelength.value());
    _colorPhase.truncate((M_TWOPI * _colorWavelength.value()));
    _hueSlicePhase.truncate(1.0);
  }

  inline ARGB colorForPixel(int pixel, const Context &context) {
    float sinOffsetBase = ((float) pixel - (float) stripLength() / 2.0f) * M_TWOPI / _wavelength.value();

    float sinOffsetV = sinOffsetBase + _lightnessPhase.value() / _wavelength.value();
//        float v = (sinf(sinOffsetV) + 1) * 0.5f;
    float v = sawtooth(sinOffsetV / M_TWOPI);
    v *= v;

    float sinOffsetBaseH = ((float) pixel - (float) stripLength() / 2.0f) * M_TWOPI / _colorWavelength.value();

    float sinOffsetH = sinOffsetBaseH + _colorPhase.value() / _colorWavelength.value();

    float sinOffsetAdjustedH = sinOffsetH / M_TWOPI;

    float hue = calculateHue(sinOffsetAdjustedH);


    return ((RGBLinear) (HSV{hue, 1.0, v * _brightness.value()})).convertWithJitter(generator);
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
  SmoothLinearControl _brightness = SmoothLinearControl(0.0, 1.0, 0.5);
  SmoothAccumulatorControl _lightnessPhase = SmoothAccumulatorControl(1000.0, -1300.0,
                                                                      16); // This should probably be an accumulator
  SmoothAccumulatorControl _colorPhase = SmoothAccumulatorControl(1000.0, -1300.0, 19);
  SmoothAccumulatorControl _hueSlicePhase = SmoothAccumulatorControl(0.0001, 0.035, 0.0025);

  std::mt19937 generator = std::mt19937(0);
  SmoothLinearControl _wavelength = SmoothLinearControl(50, 200, 150);
  SmoothLinearControl _colorWavelength = SmoothLinearControl(20, 60, 27);
//    SmoothLinearControl _centerOfWaveControl = SmoothLinearControl(-stripLength(), 0);
  SmoothLinearControl _hueSliceSizeControl = SmoothLinearControl(0.01, 0.33, .18);
//    SmoothLinearControl _saturationControl = SmoothLinearControl(1, 1);

  float _hueSliceMin = 0;


  float _hueSliceMax = 0;
  const std::vector<Control *> _controls = {
          &_brightness,
          &_lightnessPhase,
          &_colorPhase,
          &_wavelength,
          &_hueSlicePhase,
          &_hueSliceSizeControl,
          &_colorWavelength,
//        &_centerOfWaveControl,
//        &_saturationControl,
  };
};


#endif //TREELIGHTS_SINWAVESEQUENCE_H
