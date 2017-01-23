#ifndef INCLUDE_SEQUENCE_BASE_H
#define INCLUDE_SEQUENCE_BASE_H

#include "Context.h"
#include "clock.h"
#include <vector>

class Control;

class Sequence {
public:
    virtual void loop(Context *context) = 0;

    virtual void initialize() {
    }
    
    /// You should return your controls here. Default implementation is empty vector
    virtual const std::vector<Control *> &controls() {
        static const std::vector<Control *> emptyImplementation {};
        
        return emptyImplementation;
    }
};

// See ExampleSequence.cpp for exampple
template<class T>
class SequenceBase : public Sequence {
public:
    SequenceBase(int stripCount, int stripLength, const Clock &clock)
            :  _clock(clock), _stripCount(stripCount), _stripLength(stripLength) {
                
    }

    virtual void loop(Context *context) {
        bool repeatStrips = this->repeatStrips();
        int numLoops = repeatStrips ? 1 : _stripCount;
        for (int strip = 0; strip < numLoops; ++strip) {
            for (int pixel = 0; pixel < _stripLength; ++pixel) {
                ARGB c = static_cast<T *>(this)->colorForPixel(strip, pixel, *context);
                if (repeatStrips) {
                    for (int i = 0; i < _stripCount; i++) {
                        context->setColor(i, pixel, c);
                    }
                } else {
                    context->setColor(strip, pixel, c);
                }
            }
        }
    };
    
    int stripCount() {
        return _stripCount;
    }
    
    int stripLength() {
        return _stripLength;
    }
    
    
    // If this is true, we'll only calculate for 1 strip and repeat it on all 8. This is an optimization
    virtual bool repeatStrips() {
        return false;
    }
    
private:
    const Clock &_clock;
    const int _stripCount;
    const int _stripLength;
};

#endif