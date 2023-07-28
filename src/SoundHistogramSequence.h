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

  SoundData _soundData;

  const std::vector<Control *> _controls = {
  };

public:
  SoundHistogramSequence(int stripLength,
                         const Clock &clock)
          : SequenceBase<SoundHistogramSequence>::SequenceBase(stripLength, clock) {
  }


  inline ARGB colorForPixel(int pixel, const Context &context) {
//    return HSV{std::max(0.0f, std::min(1.0f, fabsf(fmodf(_hControl.value(), 1.0)))),
//               std::max(0.0f, std::min(1.0f, _sControl.value())),
//               std::max(0.0f, std::min(1.0f, 1)};
//    float value = _innerControls[bucket].value();
//   return HSV{std::max(0.0f, std::min(1.0f, (1.0f - (float)pixel / stripLength()))) * .2 + .6,
    const int length = stripLength();
    const int halfLength = length / 2;
    float hueStart = std::max<float>(0.0f,
                                     std::min(1.0f,
                                              (float) (pixel) / length)
//                                              (float) (halfLength - std::abs(pixel - halfLength)) / halfLength)
    );
    return HSV{
            hueStart * 1.2f + .2f,
            1.0f,
            _soundData.getValueForPixel(length - pixel, length)};
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