#ifndef INCLUDE_CONTEXT_H
#define INCLUDE_CONTEXT_H

//#include <assert.h>
#include "Color.h"

/// What you use to draw
class Context {
public:
    Context(ARGB *leds, int stripLength, bool reverse) :
    _leds(leds),
    _reverse(reverse),
    _stripLength(stripLength) {
    }
    
    inline void setColor(int pixel, ARGB color) {
        if (_reverse) {
            _leds[_stripLength - pixel - 1] = color;
        } else {
            _leds[pixel] = color;
        }
    }

    Context() = delete;

private:
    ARGB *_leds;

    bool _reverse;
    int _stripLength;
};

#endif