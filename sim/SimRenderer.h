#ifndef TREELIGHTS_SIM_SIMRENDERER_H
#define TREELIGHTS_SIM_SIMRENDERER_H

#include <cstdint>
#include <vector>

#include "sim/SimEngine.h"

struct RenderParams {
  float exposure = 2.2f;
  float headroom = 1.0f;  // 1.0 = SDR white; >1 = EDR nits on HDR displays
  float yaw = 0.0f;       // ball fixture only
  float pitch = 0.0f;
};

/// CPU glow compositor + tone mapper, shared by the mac app (via the ObjC++
/// bridge) and the cross-platform render CLI / snapshot tests. Pure C++,
/// clang/libc++ only (project policy).
class SimRenderer {
public:
  explicit SimRenderer(SimFixture fixture);

  int width() const { return _width; }
  int height() const { return _height; }

  /// Composites the LED state into the internal linear accumulation buffer.
  void composite(const ARGB *leds, int ledCount, float yaw, float pitch);

  /// Tone maps into extended-range *linear* sRGB RGBA16F (width*height*4
  /// halves, stored as uint16). This is what the Metal layer consumes.
  void toneMapRGBA16F(float exposure, float headroom, uint16_t *out) const;

  /// Tone maps (headroom pinned to 1.0) and sRGB-encodes into RGB8
  /// (width*height*3 bytes) for PNG output.
  void toneMapSRGB8(float exposure, uint8_t *out) const;

  /// Convenience: composite + toneMapSRGB8 into a fresh buffer.
  std::vector<uint8_t> renderSRGB8(const ARGB *leds, int ledCount,
                                   const RenderParams &params);

private:
  void compositeBars(const ARGB *leds, int ledCount);
  void compositeBall(const ARGB *leds, int ledCount, float yaw, float pitch);

  const SimFixture _fixture;
  int _width;
  int _height;
  std::vector<float> _accum;  // linear RGB
};

#endif  // TREELIGHTS_SIM_SIMRENDERER_H
