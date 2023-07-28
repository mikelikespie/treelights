#ifndef INCLUDE_SEQUENCE_BASE_H
#define INCLUDE_SEQUENCE_BASE_H

#include "Context.h"
#include "clock.h"
#include "SoundData.h"
#include <vector>

class Control;

class Sequence {
public:
  virtual void loop(Context *context) = 0;

  virtual void initialize() {
  }

  /// You should return your controls here. Default implementation is empty vector
  virtual const std::vector<Control *> &controls() {
    static const std::vector<Control *> emptyImplementation{};

    return emptyImplementation;
  }

  /// Can optionally implement to handle data from sound ffts.
  virtual void updateSoundData(const SoundDataBuffer data) {
    // Default is a noop
  }
};

// See ExampleSequence.cpp for exampple
template<class T>
class SequenceBase : public Sequence {
public:
  SequenceBase(int stripLength, const Clock &clock)
          : _clock(clock), _stripLength(stripLength) {

  }

  virtual void loop(Context *context) {
    for (int pixel = 0; pixel < _stripLength; ++pixel) {
      ARGB c = static_cast<T *>(this)->colorForPixel(pixel, *context);
      context->setColor(pixel, c);
    }
  };

  int stripLength() {
    return _stripLength;
  }


  const Clock &clock() {
    return _clock;
  }

private:
  const Clock &_clock;
  const int _stripLength;
};

#endif