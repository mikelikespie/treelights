//
// Created by Michael Lewis on 2019-04-11.
//

#ifndef TREELIGHTS_LEDMATH_H
#define TREELIGHTS_LEDMATH_H

static inline float sawtooth(float x) {
  return fabsf((roundf(x) - x)) * 2;
}

#endif //TREELIGHTS_MATH_H
