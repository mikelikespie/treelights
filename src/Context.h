#ifndef INCLUDE_CONTEXT_H
#define INCLUDE_CONTEXT_H

//#include <assert.h>
#include "Color.h"

/// What you use to draw
class Context {
public:
    Context(ARGB *leds, int stripLength, int stripCount) :
    _leds(leds),
    _stripLength(stripLength),
    _stripCount(stripCount) {
    }
    
    inline void setColor(int strip, int pixel, ARGB color) {
        _leds[strip * _stripLength + pixel] = color;
    }

    Context() = delete;

private:
    ARGB *_leds;

    int _stripLength;
    int _stripCount;
};

#endif