#import "sim/TLEngineBridge.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>

#include "sim/SimEngine.h"

namespace {

constexpr int kMarginX = 28;
constexpr int kMarginY = 24;
constexpr int kCellX = 10;
constexpr int kRowH = 46;
constexpr int kCanvasW = kMarginX * 2 + SimEngine::kStripLength * kCellX;
constexpr int kCanvasH = kMarginY * 2 + SimEngine::kStripCount * kRowH;

// Glow sprite: hot gaussian core + wide exponential halo. Peak > 1 so LED
// centers read as emissive point sources in HDR.
constexpr int kKernelRadius = 13;
constexpr int kKernelSize = kKernelRadius * 2 + 1;

const float *glowKernel() {
  static float kernel[kKernelSize * kKernelSize];
  static bool initialized = [] {
    for (int y = 0; y < kKernelSize; y++) {
      for (int x = 0; x < kKernelSize; x++) {
        float dx = (float) (x - kKernelRadius);
        float dy = (float) (y - kKernelRadius);
        float d = sqrtf(dx * dx + dy * dy);
        kernel[y * kKernelSize + x] =
            1.35f * expf(-(d * d) / (2 * 3.4f * 3.4f)) + 0.10f * expf(-d / 5.5f);
      }
    }
    return true;
  }();
  (void) initialized;
  return kernel;
}

}  // namespace

@implementation TLEngine {
  std::unique_ptr<SimEngine> _engine;
  std::unique_ptr<float[]> _accum;  // linear RGB accumulation buffer
}

+ (NSInteger)stripCount {
  return SimEngine::kStripCount;
}

+ (NSInteger)stripLength {
  return SimEngine::kStripLength;
}

+ (NSInteger)canvasWidth {
  return kCanvasW;
}

+ (NSInteger)canvasHeight {
  return kCanvasH;
}

- (instancetype)init {
  if ((self = [super init])) {
    _engine = std::make_unique<SimEngine>();
    _accum = std::make_unique<float[]>((size_t) kCanvasW * kCanvasH * 3);
  }
  return self;
}

- (NSInteger)sequenceCount {
  return _engine->sequenceCount();
}

- (NSInteger)sequenceIndex {
  return _engine->sequenceIndex();
}

- (NSString *)sequenceNameAtIndex:(NSInteger)index {
  return @(_engine->sequenceName((int) index));
}

- (void)selectSequence:(NSInteger)index {
  _engine->setSequenceIndex((int) index);
}

- (NSInteger)controlCount {
  return _engine->controlCount();
}

- (void)setControl:(NSInteger)channel value:(float)value {
  _engine->setControlValue((int) channel, value);
}

- (void)tickAtMillis:(uint32_t)millis soundBins:(const float *)bins {
  _engine->tick(millis, bins);
}

- (void)renderWithExposure:(float)exposure headroom:(float)headroom into:(void *)rgba16f {
  float *accum = _accum.get();
  memset(accum, 0, (size_t) kCanvasW * kCanvasH * 3 * sizeof(float));

  const float *kernel = glowKernel();
  const ARGB *leds = _engine->leds();

  for (int strip = 0; strip < SimEngine::kStripCount; strip++) {
    const int cy = kMarginY + strip * kRowH + kRowH / 2;
    for (int i = 0; i < SimEngine::kStripLength; i++) {
      const ARGB &led = leds[strip * SimEngine::kStripLength + i];
      const int cx = kMarginX + i * kCellX + kCellX / 2;

      // Faint socket dot so unlit strips are still visible.
      float *center = &accum[((size_t) cy * kCanvasW + cx) * 3];
      center[0] += 0.004f;
      center[1] += 0.004f;
      center[2] += 0.004f;

      // Fold in the APA102 5-bit global brightness like writeColor does.
      const float global = std::min<uint8_t>(led.a, 31) / 31.0f;
      const float r = global * led.r / 255.0f;
      const float g = global * led.g / 255.0f;
      const float b = global * led.b / 255.0f;
      if (r + g + b < 1e-4f) {
        continue;
      }

      for (int ky = 0; ky < kKernelSize; ky++) {
        const int y = cy + ky - kKernelRadius;
        if (y < 0 || y >= kCanvasH) continue;
        const float *krow = &kernel[ky * kKernelSize];
        float *row = &accum[((size_t) y * kCanvasW + cx - kKernelRadius) * 3];
        for (int kx = 0; kx < kKernelSize; kx++) {
          const int x = cx + kx - kKernelRadius;
          if (x < 0 || x >= kCanvasW) continue;
          const float k = krow[kx];
          row[kx * 3 + 0] += k * r;
          row[kx * 3 + 1] += k * g;
          row[kx * 3 + 2] += k * b;
        }
      }
    }
  }

  // Tone map into extended-range linear fp16: linear until values approach
  // the display headroom, then soft roll-off so nothing hard-clips.
  __fp16 *out = (__fp16 *) rgba16f;
  const float invHeadroom = 1.0f / std::max(headroom, 0.01f);
  const size_t pixelCount = (size_t) kCanvasW * kCanvasH;
  for (size_t p = 0; p < pixelCount; p++) {
    const float *src = &accum[p * 3];
    __fp16 *dst = &out[p * 4];
    for (int c = 0; c < 3; c++) {
      float v = src[c] * exposure;
      if (v <= 0.0f) {
        dst[c] = (__fp16) 0.0f;
      } else if (v < 0.02f) {
        dst[c] = (__fp16) v;  // linear region; skip the expf
      } else {
        dst[c] = (__fp16)(headroom * (1.0f - expf(-v * invHeadroom)));
      }
    }
    dst[3] = (__fp16) 1.0f;
  }
}

@end
