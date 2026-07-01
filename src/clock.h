#ifndef INCLUDE_CLOCK_H
#define INCLUDE_CLOCK_H

#include <stdint.h>

#if defined(ARDUINO) || defined(TEENSYDUINO)
#include <core_pins.h>
#endif

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

    inline bool is_first_frame() const {
        return delta_t == 0;
    }

    // Core update, separated from millis() so host tests can drive time.
    inline void tickWithMillis(uint32_t new_t) {
        if (last_t != 0) {
            delta_t = new_t - last_t;
            deltaf_t = delta_t / 1000.0f;
        }

        last_t = new_t;
    }

#if defined(ARDUINO) || defined(TEENSYDUINO)
    inline void tick() {
        tickWithMillis(millis());
    }
#endif

    static const Clock defaultClock;

private:
    // If its the first frame
    uint32_t last_t = 0;
    uint32_t delta_t = 0;
    float deltaf_t = 0;
};

#endif
