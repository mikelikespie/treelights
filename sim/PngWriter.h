#ifndef TREELIGHTS_SIM_PNGWRITER_H
#define TREELIGHTS_SIM_PNGWRITER_H

#include <cstdint>
#include <string>
#include <vector>

/// Writes an 8-bit RGB PNG (width*height*3 bytes).
bool WritePng(const std::string &path, const uint8_t *rgb, int width, int height,
              std::string *error);

/// Writes an animated PNG (APNG) of full-size 8-bit RGB frames with a fixed
/// per-frame delay. Plays in every browser and most viewers; still a valid
/// PNG (first frame) elsewhere.
bool WriteApng(const std::string &path, const std::vector<std::vector<uint8_t>> &frames,
               int width, int height, int delayMillis, std::string *error);

/// Incremental APNG writer so long clips never buffer all frames in memory.
/// The frame count must be known up front (the acTL chunk requires it).
class ApngWriter {
public:
  ~ApngWriter();

  bool begin(const std::string &path, int width, int height, int frameCount,
             int delayMillis, std::string *error);
  bool addFrame(const uint8_t *rgb, std::string *error);
  bool finish(std::string *error);

private:
  void *_file = nullptr;  // FILE*
  int _width = 0;
  int _height = 0;
  int _frameCount = 0;
  int _delayMillis = 0;
  int _framesWritten = 0;
  uint32_t _sequence = 0;
};

/// Reads a PNG produced by WritePng (8-bit RGB, non-interlaced, filter 0).
/// Not a general-purpose decoder — snapshot-test use only.
bool ReadPng(const std::string &path, std::vector<uint8_t> *rgb, int *width, int *height,
             std::string *error);

#endif  // TREELIGHTS_SIM_PNGWRITER_H
