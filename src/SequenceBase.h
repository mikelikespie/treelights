#ifndef INCLUDE_SEQUENCE_BASE_H
#define INCLUDE_SEQUENCE_BASE_H

#include "Context.h"
#include "clock.h"
#include "SoundData.h"
#include <vector>

class Control;

static const std::vector<Control *> emptyControlsVector {};

class Sequence {

public:
  virtual void loop(Context *context) = 0;

  virtual void initialize() {
  }

  /// You should return your controls here. Default implementation is empty vector
  virtual const std::vector<Control *> &controls() {
    return emptyControlsVector;
  }

  /// Can optionally implement to handle data from sound ffts.
  virtual void updateSoundData(const SoundDataBuffer data) {
    // Default is a noop
  }

  virtual ~Sequence() = default;
};

// See ExampleSequence.cpp for exampple
template<class T>
class SequenceBase : public Sequence {
public:
  SequenceBase(int stripLength, const Clock &clock)
          : _clock(clock), _stripLength(stripLength) {

  }

  void loop(Context *context) override {
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

  ~SequenceBase() override = default;

private:
  const Clock &_clock;
  const int _stripLength;
};

#endif