//
// Created by Michael Lewis on 8/19/15.
//

#ifndef TREELIGHTS_SINWAVESEQUENCE_H
#define TREELIGHTS_SINWAVESEQUENCE_H

#include <vector>

#include "Control.h"
#include "SequenceBase.h"

#include <math.h>

typedef BufferedControl<LinearlyInterpolatedValueControl<float>> SmoothLinearControl;

typedef AccumulatorControl<SmoothLinearControl> SmoothAccumulatorControl;


inline float sawtooth(float x) {
    return fabsf((roundf(x) - x)) * 2;
}

class SinWaveSequence : public SequenceBase<SinWaveSequence> {


public:
    SinWaveSequence(int stripCount, int stripLength, const Clock &clock)
            : SequenceBase(stripCount, stripLength,
                           clock) { }

    virtual void loop(Context *context) {
        
        _hueSliceMin = _hueSlicePhase.value() - _hueSliceSizeControl.value() * .5f;
        _hueSliceMax = _hueSlicePhase.value() + _hueSliceSizeControl.value() * .5f;

        SequenceBase<SinWaveSequence>::loop(context);

        _lightnessPhase.truncate(M_TWOPI * _wavelength.value());
        _colorPhase.truncate((M_TWOPI * _colorWavelength.value()));
        _hueSlicePhase.truncate(1.0);
    }

    inline ARGB colorForPixel(int strip, int pixel, const Context &context) {
        float sinOffsetBase = (pixel + _centerOfWaveControl.value()) * M_TWOPI / _wavelength.value();

        float sinOffsetV = sinOffsetBase +  _lightnessPhase.value() / _wavelength.value();
        float v = (sinf(sinOffsetV) + 1) * 0.5f;
        

        float sinOffsetBaseH = (pixel + _centerOfWaveControl.value()) * M_TWOPI / _colorWavelength.value();

        float sinOffsetH = sinOffsetBaseH + _colorPhase.value() / _colorWavelength.value();

        float sinOffsetAdjustedH = sinOffsetH / M_TWOPI;
        
        float hue = (sawtooth(sinOffsetAdjustedH)) * (_hueSliceMax - _hueSliceMin) + _hueSliceMin;

//        float hue = _hueSliceMin;

        if (hue < 0) {
            hue -= floor(hue);
        } else {
            hue = fmodf(hue, 1.0);
        }


        return HCL { hue, 1, v };
//        return ARGB { 8, 255, 0, 0 };
    }


    virtual const std::vector<Control *> &controls() {
        return _controls;
    }
private:
    SmoothAccumulatorControl _lightnessPhase = SmoothAccumulatorControl(1.0, 40.0); // This should probably be an accumulator
    SmoothAccumulatorControl _colorPhase = SmoothAccumulatorControl(0.0, 0.125);
    SmoothAccumulatorControl _hueSlicePhase = SmoothAccumulatorControl(0.0, 0.25);

    SmoothLinearControl _wavelength = SmoothLinearControl(4, 60);
    SmoothLinearControl _colorWavelength = SmoothLinearControl(4, 120);
    SmoothLinearControl _centerOfWaveControl = SmoothLinearControl(-stripLength(), 0);
    SmoothLinearControl _hueSliceSizeControl = SmoothLinearControl(0.0, 0.25);
    SmoothLinearControl _saturationControl = SmoothLinearControl(1, 1);
    
    float _hueSliceMin = 0;
    float _hueSliceMax = 0;
    const std::vector<Control *> _controls = {
        &_lightnessPhase,
        &_colorPhase,
        &_wavelength,
        &_hueSlicePhase,
        &_hueSliceSizeControl,
        &_colorWavelength,
        &_centerOfWaveControl,
        &_saturationControl,
    };
};


#endif //TREELIGHTS_SINWAVESEQUENCE_H
