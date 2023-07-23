//
// Created by Michael Lewis on 8/17/15.
//

#ifndef TREELIGHTS_SOUNDHISTOGRAM_SEQUENCE_H
#define TREELIGHTS_SOUNDHISTOGRAM_SEQUENCE_H


#include "SequenceBase.h"
#include "Control.h"
#include <vector>

/// Just copy the next 2 lines and replace the words "ExampleSequence"

class SoundHistogramSequence : public SequenceBase<SoundHistogramSequence> {
public:
  SoundHistogramSequence(int stripLength,
                         const Clock &clock)
          : SequenceBase<SoundHistogramSequence>::SequenceBase(stripLength, clock) {
  }


  inline ARGB colorForPixel(int pixel, const Context &context) {
//    return HSV{std::max(0.0f, std::min(1.0f, fabsf(fmodf(_hControl.value(), 1.0)))),
//               std::max(0.0f, std::min(1.0f, _sControl.value())),
//               std::max(0.0f, std::min(1.0f, 1)};
    int bucket = int(((float) pixel / stripLength()) * 32);
    float value = _innerControls[bucket].value();
//   return HSV{std::max(0.0f, std::min(1.0f, (1.0f - (float)pixel / stripLength()))) * .2 + .6,
    return HSV{std::max(0.0f, std::min(1.0f, (1.0f - (float) pixel / stripLength()))) * .4 + .6,
//    return HSV{.8,
               1.0f,
               value};
  }

  virtual const std::vector<Control *> &controls() {
    return _controls;
  }

private:

  SmoothLinearControl _innerControls[32] = {
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
          SmoothLinearControl(0, 1),
  };


  const std::vector<Control *> _controls = {&_innerControls[0],
                                            &_innerControls[1],
                                            &_innerControls[2],
                                            &_innerControls[3],
                                            &_innerControls[4],
                                            &_innerControls[5],
                                            &_innerControls[6],
                                            &_innerControls[7],
                                            &_innerControls[8],
                                            &_innerControls[9],
                                            &_innerControls[10],
                                            &_innerControls[11],
                                            &_innerControls[12],
                                            &_innerControls[13],
                                            &_innerControls[14],
                                            &_innerControls[15],
                                            &_innerControls[16],
                                            &_innerControls[17],
                                            &_innerControls[18],
                                            &_innerControls[19],
                                            &_innerControls[20],
                                            &_innerControls[21],
                                            &_innerControls[22],
                                            &_innerControls[23],
                                            &_innerControls[24],
                                            &_innerControls[25],
                                            &_innerControls[26],
                                            &_innerControls[27],
                                            &_innerControls[28],
                                            &_innerControls[29],
                                            &_innerControls[30],
                                            &_innerControls[31],
  };
};


#endif //TREELIGHTS_EXAMPLESEQUENCE_H
