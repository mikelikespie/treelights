#include "sim/AudioFile.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "SoundData.h"

namespace {

constexpr int kFftSize = 1024;

uint32_t readU32(const uint8_t *p) {
  return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) |
         ((uint32_t) p[3] << 24);
}

uint16_t readU16(const uint8_t *p) {
  return (uint16_t) (p[0] | (p[1] << 8));
}

/// In-place iterative radix-2 FFT (double precision, fixed N=1024).
void fft1024(double *re, double *im) {
  const int n = kFftSize;
  // Bit-reversal permutation.
  for (int i = 1, j = 0; i < n; i++) {
    int bit = n >> 1;
    for (; j & bit; bit >>= 1) {
      j ^= bit;
    }
    j ^= bit;
    if (i < j) {
      std::swap(re[i], re[j]);
      std::swap(im[i], im[j]);
    }
  }
  for (int len = 2; len <= n; len <<= 1) {
    const double angle = -2.0 * M_PI / len;
    const double wRe = cos(angle), wIm = sin(angle);
    for (int i = 0; i < n; i += len) {
      double curRe = 1, curIm = 0;
      for (int k = 0; k < len / 2; k++) {
        const int a = i + k, b = i + k + len / 2;
        const double tRe = re[b] * curRe - im[b] * curIm;
        const double tIm = re[b] * curIm + im[b] * curRe;
        re[b] = re[a] - tRe;
        im[b] = im[a] - tIm;
        re[a] += tRe;
        im[a] += tIm;
        const double nextRe = curRe * wRe - curIm * wIm;
        curIm = curRe * wIm + curIm * wRe;
        curRe = nextRe;
      }
    }
  }
}

}  // namespace

bool ReadWav(const std::string &path, AudioData *out, std::string *error) {
  FILE *f = fopen(path.c_str(), "rb");
  if (f == nullptr) {
    if (error) *error = "cannot open: " + path;
    return false;
  }
  std::vector<uint8_t> bytes;
  uint8_t buf[65536];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
    bytes.insert(bytes.end(), buf, buf + n);
  }
  fclose(f);

  if (bytes.size() < 12 || memcmp(bytes.data(), "RIFF", 4) != 0 ||
      memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
    if (error) *error = "not a WAV file: " + path;
    return false;
  }

  int format = 0, channels = 0, sampleRate = 0, bitsPerSample = 0;
  const uint8_t *data = nullptr;
  size_t dataLen = 0;

  size_t pos = 12;
  while (pos + 8 <= bytes.size()) {
    const uint8_t *chunk = &bytes[pos];
    uint32_t len = readU32(chunk + 4);
    const uint8_t *payload = chunk + 8;
    if (pos + 8 + len > bytes.size()) break;
    if (memcmp(chunk, "fmt ", 4) == 0 && len >= 16) {
      format = readU16(payload);
      channels = readU16(payload + 2);
      sampleRate = (int) readU32(payload + 4);
      bitsPerSample = readU16(payload + 14);
    } else if (memcmp(chunk, "data", 4) == 0) {
      data = payload;
      dataLen = len;
    }
    pos += 8 + len + (len & 1);  // chunks are word-aligned
  }

  if (data == nullptr || channels <= 0 || sampleRate <= 0) {
    if (error) *error = "malformed WAV (missing fmt/data): " + path;
    return false;
  }

  const bool pcm16 = format == 1 && bitsPerSample == 16;
  const bool pcm24 = format == 1 && bitsPerSample == 24;
  const bool float32 = format == 3 && bitsPerSample == 32;
  if (!pcm16 && !pcm24 && !float32) {
    if (error) {
      *error = "unsupported WAV encoding (want 16/24-bit PCM or 32-bit float): " + path;
    }
    return false;
  }

  const int bytesPerSample = bitsPerSample / 8;
  const size_t frameBytes = (size_t) bytesPerSample * channels;
  const size_t frameCount = dataLen / frameBytes;

  out->sampleRate = sampleRate;
  out->samples.resize(frameCount);
  for (size_t i = 0; i < frameCount; i++) {
    float sum = 0;
    for (int c = 0; c < channels; c++) {
      const uint8_t *s = data + i * frameBytes + (size_t) c * bytesPerSample;
      if (pcm16) {
        int16_t v = (int16_t) (s[0] | (s[1] << 8));
        sum += v / 32768.0f;
      } else if (pcm24) {
        int32_t v = (s[0] << 8) | (s[1] << 16) | (s[2] << 24);  // sign-extend via <<8
        sum += (v >> 8) / 8388608.0f;
      } else {
        float v;
        memcpy(&v, s, 4);
        sum += v;
      }
    }
    out->samples[i] = sum / channels;
  }
  return true;
}

bool WriteWavMono16(const std::string &path, const std::vector<float> &samples,
                    int sampleRate, std::string *error) {
  const uint32_t dataLen = (uint32_t) (samples.size() * 2);
  std::vector<uint8_t> out;
  auto pushU32 = [&](uint32_t v) {
    out.push_back((uint8_t) v);
    out.push_back((uint8_t) (v >> 8));
    out.push_back((uint8_t) (v >> 16));
    out.push_back((uint8_t) (v >> 24));
  };
  auto pushU16 = [&](uint16_t v) {
    out.push_back((uint8_t) v);
    out.push_back((uint8_t) (v >> 8));
  };

  out.insert(out.end(), {'R', 'I', 'F', 'F'});
  pushU32(36 + dataLen);
  out.insert(out.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
  pushU32(16);
  pushU16(1);  // PCM
  pushU16(1);  // mono
  pushU32((uint32_t) sampleRate);
  pushU32((uint32_t) sampleRate * 2);  // byte rate
  pushU16(2);                          // block align
  pushU16(16);                         // bits
  out.insert(out.end(), {'d', 'a', 't', 'a'});
  pushU32(dataLen);
  for (float s : samples) {
    float clamped = std::max(-1.0f, std::min(1.0f, s));
    int16_t v = (int16_t) lrintf(clamped * 32767.0f);
    pushU16((uint16_t) v);
  }

  FILE *f = fopen(path.c_str(), "wb");
  if (f == nullptr) {
    if (error) *error = "cannot open for writing: " + path;
    return false;
  }
  size_t written = fwrite(out.data(), 1, out.size(), f);
  fclose(f);
  if (written != out.size()) {
    if (error) *error = "short write: " + path;
    return false;
  }
  return true;
}

void WavFftBins512(const AudioData &audio, double tSeconds, float gain,
                   float outBins512[]) {
  double re[kFftSize], im[kFftSize];
  const long end = (long) (tSeconds * audio.sampleRate);

  for (int i = 0; i < kFftSize; i++) {
    const long idx = end - kFftSize + i;
    double sample =
        (idx >= 0 && idx < (long) audio.samples.size()) ? audio.samples[idx] : 0.0;
    // Hann window.
    const double w = 0.5 * (1.0 - cos(2.0 * M_PI * i / kFftSize));
    re[i] = sample * w;
    im[i] = 0.0;
  }

  fft1024(re, im);

  for (int bin = 0; bin < SOUND_BUFFER_BIN_COUNT; bin++) {
    const double mag = sqrt(re[bin] * re[bin] + im[bin] * im[bin]);
    outBins512[bin] = (float) (mag * 2.0 / kFftSize) * gain;
  }
}
