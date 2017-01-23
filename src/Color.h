//
// Created by Michael Lewis on 1/22/17.
//

#ifndef TREELIGHTS_COLOR_H_H
#define TREELIGHTS_COLOR_H_H

#include <stdint.h>
#include <cmath>
#include <algorithm>
#include <core_pins.h>
#include <usb_serial.h>

struct ARGB {
    uint8_t a;
    // We actually only get 5 bits
    uint8_t r, g, b;
};


// These are all 0-1
struct HCL {
    float h;
    float c;
    float l;

    operator const ARGB() const {
//
//        if (Serial) {
//            Serial.print(h);
//            Serial.print(", ");
//            Serial.print(c);
//            Serial.print(", ");
//            Serial.println(h);
//        }

//        return ARGB { 8, 255, 0, 0 };

        // quadrant is [0,6)
        // TODO support negative hues if we have to

        float c = l * this->c;

        float h_prime = std::min(1.0f, std::max(0.0f, h)) * 6;

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

        float m = l - c;

        float r = std::min(r1 + m, 1.0f);
        float g = std::min(g1 + m, 1.0f);
        float b = std::min(b1 + m, 1.0f);

        float actual_l = r * r + g * g + b * b;

        float multiple = l / sqrtf(actual_l);

        r *= multiple;
        g *= multiple;
        b *= multiple;

        g = (g * 0.75) * (g * 0.75);
        r *= r;
        b *= b;

        const  float threshold = 2;

        uint8_t a = 31;
        float maxrgb = std::max(std::max(r, g), b);

        if (maxrgb < threshold) {
            if (maxrgb == 0) {
                return ARGB {0, 0, 0, 0};
            }

//            float multiple = std::min(floor(maxrgb / (256 * 32), 32);
            float needed_mult = 1.0 / maxrgb;
            a = uint8_t(ceilf(31 / needed_mult));

            float actual_mult = a / 31.0f;
            r /= actual_mult;
            g /= actual_mult;
            b /= actual_mult;
        } else {
//            a = 0;
        }
        return ARGB {a, uint8_t(roundf(r * 255.0)), uint8_t(roundf(g * 255.0)), uint8_t(roundf(b * 255.0))};

    }
};

#endif //TREELIGHTS_COLOR_H_H
