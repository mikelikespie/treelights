//
// Created by Michael Lewis on 8/17/15.
//

#ifndef TREELIGHTS_SOUNDHISTOGRAM_SEQUENCE_H
#define TREELIGHTS_SOUNDHISTOGRAM_SEQUENCE_H


#include "SequenceBase.h"
#include "Control.h"
#include "SoundData.h"
#include <vector>

/// Just copy the next 2 lines and replace the words "ExampleSequence"

class SoundHistogramSequence : public SequenceBase<SoundHistogramSequence> {


private:

  std::vector<float> _lastValues;

  SoundData _soundData;

  SmoothLinearControl _gain = SmoothLinearControl(0.5, 6.0, 1.0);


  float _cachedDecayAmount = 0;
//  SmoothAccumulatorControl _hueSlicePhase = SmoothAccumulatorControl(0.0001, 0.035, 0.0025);
//  SmoothLinearControl _hueSliceSizeControl = SmoothLinearControl(0.01, 0.33, .18);


  const std::vector<Control *> _controls = {
          &_gain,
//          &_hueSlicePhase,
//          &_hueSliceSizeControl,
  };

public:
  SoundHistogramSequence(int stripLength,
                         const Clock &clock)
          : SequenceBase<SoundHistogramSequence>::SequenceBase(stripLength, clock), _lastValues(stripLength) {
  }


  void loop(Context *context) override {
    const float k = 30.0f; // Magic number that seems to be low latency enough, but also reduce flashiness

    _cachedDecayAmount = expf(-clock().deltaf() * k);

    SequenceBase::loop(context);
  }

  inline ARGB colorForPixel(int pixel, __attribute__((unused)) const Context &context) {
//    return HSV{std::max(0.0f, std::min(1.0f, fabsf(fmodf(_hControl.value(), 1.0)))),
//               std::max(0.0f, std::min(1.0f, _sControl.value())),
//               std::max(0.0f, std::min(1.0f, 1)};
//    float value = _innerControls[bucket].value();
//   return HSV{std::max(0.0f, std::min(1.0f, (1.0f - (float)pixel / stripLength()))) * .2 + .6,
    const int length = stripLength();
//    const int halfLength = length / 2;
    float hueStart = std::max<float>(0.0f,
                                     std::min(1.0f,
                                              (float) (pixel) / (float) length)
//                                              (float) (halfLength - std::abs(pixel - halfLength)) / halfLength)
    );
    float lastValueForPixel = _lastValues[pixel];
    float newValueForPixel = _soundData.getValueForPixel(length - pixel, length);

    float actualValue = lastValueForPixel * _cachedDecayAmount + newValueForPixel * (1.0f - _cachedDecayAmount);

    _lastValues[pixel] = actualValue;

    return HSV{
            hueStart * 1.0f + .2f,
            1.0f,
            actualValue * _gain.value()};
  }

  virtual const std::vector<Control *> &controls() {
    return _controls;
  }

  void updateSoundData(const SoundDataBuffer data) override {
    Sequence::updateSoundData(data);
    _soundData.updateBuffer(data);
  }
};

#endif //TREELIGHTS_EXAMPLESEQUENCE_H