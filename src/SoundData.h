//
// Created by Michael Lewis on 7/26/23.
//

#ifndef TREELIGHTS_SOUNDDATA_H
#define TREELIGHTS_SOUNDDATA_H


static const int SOUND_BUFFER_BIN_COUNT = 512;

/// Sound data with all the bins from the fft
/// These are linear with with 43hz per bin
typedef float SoundDataBuffer[SOUND_BUFFER_BIN_COUNT];

/// Represents data coming from the sound fft library. has 512 bins. It is
/// it is 1024 samples from 44100hz audio. It is 23ms of audio. Each bin is 43hz
class SoundData {
public:
  /// Updates buffer
  void updateBuffer(const SoundDataBuffer buffer) {
    memcpy(_buffer, buffer, sizeof(_buffer));
  }

  /// Figures out which bin(s) the value lies in.
  /// Values are logarthmic so we need to do some math to figure out which bins,
  float getValueForPixel(int pixel, int stripLength, float minFrequency = 53.0f, float maxFrequency = 5000.0f) {

    float logMin = std::log(minFrequency);
    float logMax = std::log(maxFrequency);

    float pixelRatioPrev = (float) std::max(pixel - 1, 0) / (float) (stripLength - 1);
    float pixelRatioNext = (float) (pixel + 1) / (float) (stripLength - 1);

    float freqPrev = std::exp(pixelRatioPrev * (logMax - logMin) + logMin);
    float freqNext = std::exp(pixelRatioNext * (logMax - logMin) + logMin);

    float binPrevFloat = freqPrev / 43.0f;
    int binPrev = std::min((int)std::floor (binPrevFloat), SOUND_BUFFER_BIN_COUNT - 1);
    float binNextFloat = freqNext / 43.0f;
    int binNext = std::min((int)std::floor (binNextFloat), SOUND_BUFFER_BIN_COUNT - 1);
    if (binPrev == binNext || binNextFloat - binPrevFloat < 1.5f) {
//      return 0;
      // If we're in the last bin we don't interpolate
//      if (binPrev == SOUND_BUFFER_BIN_COUNT - 1) {
        return _buffer[binPrev];
//      } else {
//        // Linear interpolate between _buffer[binPrev] and _buffer[binPrev + 1]
//        float interpolationRatio = (freqPrev - binPrev * 43.0f) / 43.0f;
//        return _buffer[binPrev] * (1.0f - interpolationRatio) + _buffer[binPrev + 1] * interpolationRatio;
//      }
    } else {
//      return 1;
      float binSum = 0;
      for (int i = binPrev; i <= binNext; ++i) {
        binSum += _buffer[i];
      }

      return binSum;

    }
  }

  float getCombinedFrequencyRange(float minFrequency, float maxFrequency) {
    float logMin = std::log(minFrequency);
    float logMax = std::log(maxFrequency);

    int binMin = std::min((int) (std::exp(logMin) / 43.0f), SOUND_BUFFER_BIN_COUNT - 1);
    int binMax = std::min((int) (std::exp(logMax) / 43.0f), SOUND_BUFFER_BIN_COUNT - 1);

    float binSum = 0;
    for (int i = binMin; i <= binMax; ++i) {
      binSum += _buffer[i];
    }

    return binSum;
  }

private:
  SoundDataBuffer _buffer = {0};
};

#endif //TREELIGHTS_SOUNDDATA_H
