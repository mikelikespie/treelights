#include "sim/PngWriter.h"

#include <zlib.h>

#include <cstdio>
#include <cstring>

namespace {

void appendU32(std::vector<uint8_t> *out, uint32_t v) {
  out->push_back((uint8_t) (v >> 24));
  out->push_back((uint8_t) (v >> 16));
  out->push_back((uint8_t) (v >> 8));
  out->push_back((uint8_t) v);
}

void appendU16(std::vector<uint8_t> *out, uint16_t v) {
  out->push_back((uint8_t) (v >> 8));
  out->push_back((uint8_t) v);
}

void appendChunk(std::vector<uint8_t> *out, const char type[4],
                 const std::vector<uint8_t> &data) {
  appendU32(out, (uint32_t) data.size());
  size_t crcStart = out->size();
  out->insert(out->end(), type, type + 4);
  out->insert(out->end(), data.begin(), data.end());
  uint32_t crc = (uint32_t) crc32(0, out->data() + crcStart, (uInt) (out->size() - crcStart));
  appendU32(out, crc);
}

std::vector<uint8_t> ihdrData(int width, int height) {
  std::vector<uint8_t> ihdr;
  appendU32(&ihdr, (uint32_t) width);
  appendU32(&ihdr, (uint32_t) height);
  ihdr.push_back(8);  // bit depth
  ihdr.push_back(2);  // color type: truecolor RGB
  ihdr.push_back(0);  // compression
  ihdr.push_back(0);  // filter
  ihdr.push_back(0);  // interlace
  return ihdr;
}

/// Filter-0 scanlines, deflate-compressed.
bool compressFrame(const uint8_t *rgb, int width, int height,
                   std::vector<uint8_t> *compressed, std::string *error) {
  if (width <= 0 || height <= 0) {
    if (error) *error = "invalid image dimensions";
    return false;
  }
  std::vector<uint8_t> raw((size_t) height * (1 + (size_t) width * 3));
  for (int y = 0; y < height; y++) {
    uint8_t *row = &raw[(size_t) y * (1 + (size_t) width * 3)];
    row[0] = 0;  // filter type 0 (none)
    memcpy(row + 1, rgb + (size_t) y * width * 3, (size_t) width * 3);
  }
  uLongf bound = compressBound((uLong) raw.size());
  compressed->resize(bound);
  int rc = compress2(compressed->data(), &bound, raw.data(), (uLong) raw.size(), 6);
  if (rc != Z_OK) {
    if (error) *error = "zlib compress failed";
    return false;
  }
  compressed->resize(bound);
  return true;
}

bool writeFile(const std::string &path, const std::vector<uint8_t> &bytes,
               std::string *error) {
  FILE *f = fopen(path.c_str(), "wb");
  if (f == nullptr) {
    if (error) *error = "cannot open for writing: " + path;
    return false;
  }
  size_t written = fwrite(bytes.data(), 1, bytes.size(), f);
  fclose(f);
  if (written != bytes.size()) {
    if (error) *error = "short write: " + path;
    return false;
  }
  return true;
}

const uint8_t kPngSignature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};

}  // namespace

bool WritePng(const std::string &path, const uint8_t *rgb, int width, int height,
              std::string *error) {
  std::vector<uint8_t> compressed;
  if (!compressFrame(rgb, width, height, &compressed, error)) {
    return false;
  }

  std::vector<uint8_t> out(kPngSignature, kPngSignature + 8);
  appendChunk(&out, "IHDR", ihdrData(width, height));
  appendChunk(&out, "IDAT", compressed);
  appendChunk(&out, "IEND", {});
  return writeFile(path, out, error);
}

bool WriteApng(const std::string &path, const std::vector<std::vector<uint8_t>> &frames,
               int width, int height, int delayMillis, std::string *error) {
  ApngWriter writer;
  if (!writer.begin(path, width, height, (int) frames.size(), delayMillis, error)) {
    return false;
  }
  for (const auto &frame : frames) {
    if (!writer.addFrame(frame.data(), error)) {
      return false;
    }
  }
  return writer.finish(error);
}

ApngWriter::~ApngWriter() {
  if (_file != nullptr) {
    fclose((FILE *) _file);
  }
}

bool ApngWriter::begin(const std::string &path, int width, int height, int frameCount,
                       int delayMillis, std::string *error) {
  if (frameCount <= 0 || width <= 0 || height <= 0) {
    if (error) *error = "invalid APNG dimensions/frame count";
    return false;
  }
  // The fcTL delay numerator is a 16-bit field; reject rather than wrap.
  if (delayMillis < 0 || delayMillis > 65535) {
    if (error) *error = "APNG frame delay out of range (0..65535 ms)";
    return false;
  }

  FILE *f = fopen(path.c_str(), "wb");
  if (f == nullptr) {
    if (error) *error = "cannot open for writing: " + path;
    return false;
  }
  _file = f;
  _width = width;
  _height = height;
  _frameCount = frameCount;
  _delayMillis = delayMillis;
  _framesWritten = 0;
  _sequence = 0;

  std::vector<uint8_t> out(kPngSignature, kPngSignature + 8);
  appendChunk(&out, "IHDR", ihdrData(width, height));
  std::vector<uint8_t> actl;
  appendU32(&actl, (uint32_t) frameCount);
  appendU32(&actl, 0);  // loop forever
  appendChunk(&out, "acTL", actl);
  if (fwrite(out.data(), 1, out.size(), f) != out.size()) {
    if (error) *error = "short write";
    return false;
  }
  return true;
}

bool ApngWriter::addFrame(const uint8_t *rgb, std::string *error) {
  if (_file == nullptr || _framesWritten >= _frameCount) {
    if (error) *error = "APNG writer misuse (not begun or too many frames)";
    return false;
  }

  std::vector<uint8_t> out;
  std::vector<uint8_t> fctl;
  appendU32(&fctl, _sequence++);
  appendU32(&fctl, (uint32_t) _width);
  appendU32(&fctl, (uint32_t) _height);
  appendU32(&fctl, 0);  // x offset
  appendU32(&fctl, 0);  // y offset
  appendU16(&fctl, (uint16_t) _delayMillis);
  appendU16(&fctl, 1000);  // delay denominator
  fctl.push_back(0);       // dispose: none
  fctl.push_back(0);       // blend: source
  appendChunk(&out, "fcTL", fctl);

  std::vector<uint8_t> compressed;
  if (!compressFrame(rgb, _width, _height, &compressed, error)) {
    return false;
  }
  if (_framesWritten == 0) {
    appendChunk(&out, "IDAT", compressed);
  } else {
    std::vector<uint8_t> fdat;
    appendU32(&fdat, _sequence++);
    fdat.insert(fdat.end(), compressed.begin(), compressed.end());
    appendChunk(&out, "fdAT", fdat);
  }
  _framesWritten++;

  if (fwrite(out.data(), 1, out.size(), (FILE *) _file) != out.size()) {
    if (error) *error = "short write";
    return false;
  }
  return true;
}

bool ApngWriter::finish(std::string *error) {
  if (_file == nullptr) {
    if (error) *error = "APNG writer not begun";
    return false;
  }
  if (_framesWritten != _frameCount) {
    if (error) *error = "APNG frame count mismatch";
    return false;
  }
  std::vector<uint8_t> out;
  appendChunk(&out, "IEND", {});
  bool ok = fwrite(out.data(), 1, out.size(), (FILE *) _file) == out.size();
  ok = fclose((FILE *) _file) == 0 && ok;
  _file = nullptr;
  if (!ok && error) *error = "short write / close failed";
  return ok;
}

bool ReadPng(const std::string &path, std::vector<uint8_t> *rgb, int *width, int *height,
             std::string *error) {
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

  if (bytes.size() < 8 || memcmp(bytes.data(), kPngSignature, 8) != 0) {
    if (error) *error = "not a PNG: " + path;
    return false;
  }

  int w = 0, h = 0;
  std::vector<uint8_t> idat;
  size_t pos = 8;
  while (pos + 12 <= bytes.size()) {
    uint32_t len = (bytes[pos] << 24) | (bytes[pos + 1] << 16) | (bytes[pos + 2] << 8) |
                   bytes[pos + 3];
    const uint8_t *type = &bytes[pos + 4];
    const uint8_t *data = &bytes[pos + 8];
    if (pos + 12 + len > bytes.size()) break;
    if (memcmp(type, "IHDR", 4) == 0) {
      if (len < 13) {
        if (error) *error = "malformed IHDR chunk: " + path;
        return false;
      }
      w = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
      h = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
      if (data[8] != 8 || data[9] != 2 || data[12] != 0) {
        if (error) *error = "unsupported PNG format (want 8-bit RGB): " + path;
        return false;
      }
    } else if (memcmp(type, "IDAT", 4) == 0) {
      idat.insert(idat.end(), data, data + len);
    } else if (memcmp(type, "IEND", 4) == 0) {
      break;
    }
    pos += 12 + len;
  }

  if (w <= 0 || h <= 0 || idat.empty()) {
    if (error) *error = "malformed PNG: " + path;
    return false;
  }

  std::vector<uint8_t> raw((size_t) h * (1 + (size_t) w * 3));
  uLongf rawLen = (uLongf) raw.size();
  if (uncompress(raw.data(), &rawLen, idat.data(), (uLong) idat.size()) != Z_OK ||
      rawLen != raw.size()) {
    if (error) *error = "zlib uncompress failed: " + path;
    return false;
  }

  rgb->resize((size_t) w * h * 3);
  for (int y = 0; y < h; y++) {
    const uint8_t *row = &raw[(size_t) y * (1 + (size_t) w * 3)];
    if (row[0] != 0) {
      if (error) *error = "unsupported PNG filter (want 0): " + path;
      return false;
    }
    memcpy(rgb->data() + (size_t) y * w * 3, row + 1, (size_t) w * 3);
  }
  *width = w;
  *height = h;
  return true;
}
