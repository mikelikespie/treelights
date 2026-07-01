#include "sim/SyntheticBeat.h"

#include <cmath>
#include <cstring>

#include "SoundData.h"

void SyntheticBeatBins(double tSeconds, float outBins512[]) {
  memset(outBins512, 0, SOUND_BUFFER_BIN_COUNT * sizeof(float));

  // Four-on-the-floor kick.
  const float kick = (float) exp(-fmod(tSeconds, 0.5) * 14);
  const float kickWeights[] = {0.9f, 1.0f, 0.8f, 0.5f, 0.3f, 0.18f};
  for (int i = 0; i < 6; i++) {
    outBins512[1 + i] += kick * kickWeights[i];
  }

  // Snare on the 2 and 4.
  const float snare = (float) exp(-fmod(tSeconds + 0.5, 1.0) * 10) * 0.4f;
  for (int bin = 30; bin < 90; bin++) {
    outBins512[bin] += snare * (float) exp(-(bin - 30) / 25.0);
  }

  // Offbeat hats.
  const float hat = (float) exp(-fmod(tSeconds + 0.25, 0.5) * 24) * 0.22f;
  for (int bin = 150; bin < 380; bin++) {
    outBins512[bin] += hat;
  }

  // Slow synth swell in the mids.
  const float swell = (float) (0.5 + 0.5 * sin(tSeconds * 0.7)) * 0.12f;
  for (int bin = 10; bin < 26; bin++) {
    outBins512[bin] += swell;
  }
}
