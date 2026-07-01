// Generates the deterministic test-tone WAV used by snapshot tests and
// demo renders: 4 seconds of kick / offbeat blips / hat noise at 44.1 kHz.

#include <cmath>
#include <cstdio>
#include <vector>

#include "sim/AudioFile.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: gen_test_wav <out.wav>\n");
    return 2;
  }

  const int rate = 44100;
  const double duration = 4.0;
  std::vector<float> samples((size_t) (rate * duration), 0.0f);

  uint32_t lcg = 12345;  // deterministic hat noise
  for (size_t i = 0; i < samples.size(); i++) {
    const double t = (double) i / rate;
    double v = 0;

    // Kick: 55 Hz decaying sine every half second.
    const double kickT = fmod(t, 0.5);
    v += 0.8 * sin(2 * M_PI * 55 * kickT) * exp(-kickT * 18);

    // Offbeat 1.2 kHz blip.
    const double blipT = fmod(t + 0.25, 0.5);
    v += 0.25 * sin(2 * M_PI * 1200 * t) * exp(-blipT * 40);

    // Hat: short noise burst every quarter second.
    const double hatT = fmod(t, 0.25);
    lcg = lcg * 1664525u + 1013904223u;
    const double noise = ((int32_t) (lcg >> 8) / (double) (1 << 23)) - 1.0;
    v += 0.12 * noise * exp(-hatT * 60);

    samples[i] = (float) (v * 0.8);
  }

  std::string error;
  if (!WriteWavMono16(argv[1], samples, rate, &error)) {
    fprintf(stderr, "%s\n", error.c_str());
    return 1;
  }
  return 0;
}
