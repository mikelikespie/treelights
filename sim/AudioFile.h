#ifndef TREELIGHTS_SIM_AUDIOFILE_H
#define TREELIGHTS_SIM_AUDIOFILE_H

#include <string>
#include <vector>

struct AudioData {
  std::vector<float> samples;  // mono, -1..1
  int sampleRate = 0;
};

/// Reads a PCM WAV file (16/24-bit int or 32-bit float), downmixed to mono.
bool ReadWav(const std::string &path, AudioData *out, std::string *error);

/// Writes a mono 16-bit PCM WAV (used by the test-tone generator).
bool WriteWavMono16(const std::string &path, const std::vector<float> &samples,
                    int sampleRate, std::string *error);

/// Computes the 512 Teensy-shaped FFT bins for the 1024-sample window ending
/// at tSeconds (zero-padded before the start of the file): Hann window,
/// radix-2 FFT, magnitude * 2/N * gain. Deterministic.
void WavFftBins512(const AudioData &audio, double tSeconds, float gain,
                   float outBins512[]);

#endif  // TREELIGHTS_SIM_AUDIOFILE_H
