#ifndef INCLUDE_CLOCK_H
#define INCLUDE_CLOCK_H

#include <core_pins.h>

/*!
 * Clock that represents relative time
 */
class Clock {
public:
    // Milliseconds that have passed since last frame.
    inline uint32_t delta() const {
        return delta_t;
    }

    // Delta in floating point. this is cached so makes it a bit easier
    inline float deltaf() const {
        return deltaf_t;
    }

    inline bool is_first_frame() {
        return delta_t == 0;
    }

    inline void tick() {
        auto new_t = millis();

        if (last_t != 0) {
            delta_t = new_t - last_t;
            deltaf_t =  delta_t / 1000.0f;
        }

        last_t = new_t;
    }


    static const Clock defaultClock;

private:
    // If its the first frame
    uint32_t last_t = 0;
    uint32_t delta_t = 0;
    float deltaf_t = 0;
};

#endif