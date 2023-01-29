//
// Created by Michael Lewis on 1/22/17.
//

#ifndef TREELIGHTS_COLOR_H_H
#define TREELIGHTS_COLOR_H_H



#include <random>
#include <cmath>
#include <algorithm>

#include <Arduino.h>
#include <usb_serial.h>

#include <core_pins.h>
//
//#ifdef round
//#undef round
//#endif
//
//#ifdef abs
//#undef abs
//#endif
//
//#ifdef min
//#undef min
//#endif
//#ifdef max
//#undef max
//#endif
//

#include <stdint.h>

#ifdef abs
#undef abs
#endif

struct ARGB {
    uint8_t a;
    // We actually only get 5 bits
    uint8_t r, g, b;
};

template <typename Generator>
inline ARGB adjustLinearFloatColor(float r, float g, float b, Generator *rnd = nullptr);



template  <typename Generator>
uint8_t convertTo8bitWithJitter(float c, Generator *rnd) ;

struct RGBLinear {
    float r, g, b;

    operator const ARGB() const {
        return adjustLinearFloatColor<std::mt19937>(r, g, b, nullptr);
    }

    template <typename Generator>
    inline ARGB convertWithJitter(Generator &rnd) const {
        return adjustLinearFloatColor(r, g, b, &rnd);
    }
};

struct RGBLog {
    float r, g, b;

    inline operator const RGBLinear() const {
// THIS IS WRONG, just hardcoded it to make LEDs bright
      return RGBLinear { r , g ,  b };
//        return RGBLinear { r * r, g * g, b * b };
    }
};


// These are all 0-1
struct HSV {
    float h;
    float s;
    float v;

    inline operator const RGBLinear() const {
        float c = v * this->s;

        float h_prime = std::min<float>(1.0f, std::max<float>(0.0f, h)) * 6;

        float r1 = 0, g1 = 0, b1 = 0;

        float x = c * (1 - fabsf(fmodf(h_prime, 2.0) - 1));

        switch (uint8_t(h_prime) % 6) {
            case 0:
                r1 = c;
                g1 = x;
                break;
            case 1:
                r1 = x;
                g1 = c;
                break;
            case 2:
                g1 = c;
                b1 = x;
                break;
            case 3:
                g1 = x;
                b1 = c;
                break;
            case 4:
                r1 = x;
                b1 = c;
                break;
            case 5:
                r1 = c;
                b1 = x;
                break;

            default:
                break;
        }

        float m = v - c;

        float r = std::min(r1 + m, 1.0f);
        float g = std::min(g1 + m, 1.0f);
        float b = std::min(b1 + m, 1.0f);

        return (RGBLinear)RGBLog {r, g, b};
    };

    operator const ARGB() const {
        return (RGBLinear) (*this);
    }
};

//static const float _gamma = 2.4f;
//static const float _invGamma= 2.4f;


template <typename Generator>
inline ARGB adjustLinearFloatColor(float r, float g, float b, Generator *rnd) {
    const float threshold = 2;

    uint8_t a = 31;
    float maxrgb = std::max<float>(std::max<float>(r, g), b);

    if (maxrgb < threshold) {
        if (maxrgb == 0) {
            return ARGB {0, 0, 0, 0};
        }

        float needed_mult = 1.0f / maxrgb;
        a = uint8_t(roundf(31 / needed_mult));
        if (a == 0) {
            a = 1;
        }

        float actual_mult = 31.0f / a;
        r *= actual_mult;
        g *= actual_mult;
        b *= actual_mult;
    } else {
//            a = 0;
    }
    if (rnd == nullptr) {
        return ARGB {a, uint8_t(roundf(r * 255.0f)), uint8_t(roundf(g * 255.0f)), uint8_t(roundf(b * 255.0f))};
    } else {
        return ARGB {a,
                     convertTo8bitWithJitter(r, rnd),
                     convertTo8bitWithJitter(g, rnd),
                     convertTo8bitWithJitter(b, rnd)
        };
    }
}


const float ditherThreshold = 8;
template <typename Generator>
inline uint8_t convertTo8bitWithJitter(float c, Generator *rnd) {

    c *= 255.0f;

    if (c <= 0.005) {
        return 0;
    }

    if (c > ditherThreshold) {
        return uint8_t(roundf(std::min(c, 255.0f)));
    }

    float probablilityToFloor = c - floorf(c);
    std::uniform_real_distribution<float> dist(0.0, 1.0);
    bool shouldFloor = dist(*rnd) > probablilityToFloor;

    return (uint8_t)std::min(shouldFloor ? c : ceilf(c), 255.0f);
}

#endif //TREELIGHTS_COLOR_H_H
