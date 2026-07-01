#include "sim/SimRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace {

// ---------------------------------------------------------------------------
// Bars layout
// ---------------------------------------------------------------------------

constexpr int kMarginX = 28;
constexpr int kMarginY = 24;
constexpr int kCellX = 10;
constexpr int kRowH = 46;
constexpr int kBarsCanvasW = kMarginX * 2 + 118 * kCellX;
constexpr int kBarsCanvasH = kMarginY * 2 + 8 * kRowH;

// Glow sprite for the bars view: hot gaussian core + wide exponential halo.
// Peak > 1 so LED centers read as emissive point sources in HDR.
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

// ---------------------------------------------------------------------------
// Icosahedron ball layout
// ---------------------------------------------------------------------------

constexpr int kBallCanvasW = 820;
constexpr int kBallCanvasH = 820;
constexpr float kCameraDistance = 3.0f;
constexpr float kFocalLength = 720.0f;

struct Vec3 {
  float x, y, z;
};

/// One 3D position per LED: 30 icosahedron edges x 18 LEDs, chained in the
/// strip's wiring order. The physical harness order wasn't recorded anywhere,
/// so edges are chained greedily for visual continuity (an icosahedron has no
/// Eulerian path, so some hops are inevitable — just like the real wiring).
/// If you map out the real ball, replace the ordering below with its table.
const std::vector<Vec3> &ballLedPositions() {
  static const std::vector<Vec3> positions = [] {
    const float phi = (1.0f + sqrtf(5.0f)) / 2.0f;
    std::vector<Vec3> verts;
    for (float a : {-1.0f, 1.0f}) {
      for (float b : {-phi, phi}) {
        verts.push_back({0, a, b});
        verts.push_back({a, b, 0});
        verts.push_back({b, 0, a});
      }
    }
    for (auto &v : verts) {
      float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
      v = {v.x / len, v.y / len, v.z / len};
    }

    std::vector<std::pair<int, int>> edges;
    for (int i = 0; i < (int) verts.size(); i++) {
      for (int j = i + 1; j < (int) verts.size(); j++) {
        float dx = verts[i].x - verts[j].x;
        float dy = verts[i].y - verts[j].y;
        float dz = verts[i].z - verts[j].z;
        float d = sqrtf(dx * dx + dy * dy + dz * dz);
        if (d < 1.2f) {  // edge length ~1.05, next-nearest ~1.7
          edges.emplace_back(i, j);
        }
      }
    }

    // Greedy chain: keep extending from the current end vertex while an
    // unused edge is available there; otherwise jump to any unused edge.
    std::vector<std::pair<int, int>> ordered;
    std::vector<bool> used(edges.size(), false);
    int current = edges[0].second;
    ordered.push_back(edges[0]);
    used[0] = true;
    while (ordered.size() < edges.size()) {
      int found = -1;
      for (int e = 0; e < (int) edges.size(); e++) {
        if (used[e]) continue;
        if (edges[e].first == current || edges[e].second == current) {
          found = e;
          break;
        }
      }
      if (found < 0) {
        for (int e = 0; e < (int) edges.size(); e++) {
          if (!used[e]) {
            found = e;
            break;
          }
        }
      }
      used[found] = true;
      auto edge = edges[found];
      if (edge.second == current) {
        std::swap(edge.first, edge.second);
      }
      ordered.push_back(edge);
      current = ordered.back().second;
    }

    std::vector<Vec3> leds;
    leds.reserve(SimEngine::kBallSegmentCount * SimEngine::kBallSegmentLength);
    for (const auto &edge : ordered) {
      const Vec3 &a = verts[edge.first];
      const Vec3 &b = verts[edge.second];
      for (int i = 0; i < SimEngine::kBallSegmentLength; i++) {
        float t = (i + 0.5f) / SimEngine::kBallSegmentLength;
        leds.push_back({a.x + (b.x - a.x) * t,
                        a.y + (b.y - a.y) * t,
                        a.z + (b.z - a.z) * t});
      }
    }
    return leds;
  }();
  return positions;
}

inline void splatAnalytic(float *accum, int canvasW, int canvasH, float cx, float cy,
                          float radius, float r, float g, float b) {
  const int x0 = std::max(0, (int) (cx - radius));
  const int x1 = std::min(canvasW - 1, (int) (cx + radius));
  const int y0 = std::max(0, (int) (cy - radius));
  const int y1 = std::min(canvasH - 1, (int) (cy + radius));
  const float sigma = radius / 3.8f;
  const float invTwoSigmaSq = 1.0f / (2 * sigma * sigma);
  const float invHalo = 1.0f / (sigma * 1.6f);
  for (int y = y0; y <= y1; y++) {
    float *row = &accum[((size_t) y * canvasW) * 3];
    const float dy = y - cy;
    for (int x = x0; x <= x1; x++) {
      const float dx = x - cx;
      const float dsq = dx * dx + dy * dy;
      const float d = sqrtf(dsq);
      const float k = 1.35f * expf(-dsq * invTwoSigmaSq) + 0.10f * expf(-d * invHalo);
      row[x * 3 + 0] += k * r;
      row[x * 3 + 1] += k * g;
      row[x * 3 + 2] += k * b;
    }
  }
}

/// Shared soft roll-off toward `headroom` (linear below 0.02 to skip expf).
inline float toneMapChannel(float v, float headroom, float invHeadroom) {
  if (v <= 0.0f) {
    return 0.0f;
  }
  if (v < 0.02f) {
    return v;
  }
  return headroom * (1.0f - expf(-v * invHeadroom));
}

inline uint8_t srgbEncode(float linear) {
  float v = std::min(1.0f, std::max(0.0f, linear));
  float encoded = v <= 0.0031308f ? 12.92f * v : 1.055f * powf(v, 1.0f / 2.4f) - 0.055f;
  return (uint8_t) lrintf(encoded * 255.0f);
}

}  // namespace

SimRenderer::SimRenderer(SimFixture fixture)
    : _fixture(fixture),
      _width(fixture == SimFixture::kBall ? kBallCanvasW : kBarsCanvasW),
      _height(fixture == SimFixture::kBall ? kBallCanvasH : kBarsCanvasH) {
  _accum.assign((size_t) _width * _height * 3, 0.0f);
}

void SimRenderer::composite(const ARGB *leds, int ledCount, float yaw, float pitch) {
  memset(_accum.data(), 0, _accum.size() * sizeof(float));
  if (_fixture == SimFixture::kBall) {
    compositeBall(leds, ledCount, yaw, pitch);
  } else {
    compositeBars(leds, ledCount);
  }
}

void SimRenderer::compositeBars(const ARGB *leds, int ledCount) {
  float *accum = _accum.data();
  const float *kernel = glowKernel();
  const int stripLength = 118;
  const int stripCount = std::min(8, ledCount / stripLength);

  for (int strip = 0; strip < stripCount; strip++) {
    const int cy = kMarginY + strip * kRowH + kRowH / 2;
    for (int i = 0; i < stripLength; i++) {
      const ARGB &led = leds[strip * stripLength + i];
      const int cx = kMarginX + i * kCellX + kCellX / 2;

      // Faint socket dot so unlit strips are still visible.
      float *center = &accum[((size_t) cy * _width + cx) * 3];
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
        if (y < 0 || y >= _height) continue;
        const float *krow = &kernel[ky * kKernelSize];
        float *row = &accum[((size_t) y * _width) * 3];
        for (int kx = 0; kx < kKernelSize; kx++) {
          const int x = cx + kx - kKernelRadius;
          if (x < 0 || x >= _width) continue;
          const float k = krow[kx];
          row[x * 3 + 0] += k * r;
          row[x * 3 + 1] += k * g;
          row[x * 3 + 2] += k * b;
        }
      }
    }
  }
}

void SimRenderer::compositeBall(const ARGB *leds, int ledCount, float yaw, float pitch) {
  float *accum = _accum.data();
  const std::vector<Vec3> &positions = ballLedPositions();
  const int count = std::min(ledCount, (int) positions.size());

  const float cosYaw = cosf(yaw), sinYaw = sinf(yaw);
  const float cosPitch = cosf(pitch), sinPitch = sinf(pitch);
  const float centerX = _width / 2.0f;
  const float centerY = _height / 2.0f;

  for (int i = 0; i < count; i++) {
    const Vec3 &p = positions[i];

    // Yaw around Y, then pitch around X.
    const float x1 = p.x * cosYaw + p.z * sinYaw;
    const float z1 = -p.x * sinYaw + p.z * cosYaw;
    const float y2 = p.y * cosPitch - z1 * sinPitch;
    const float z2 = p.y * sinPitch + z1 * cosPitch;

    const float depth = z2 + kCameraDistance;  // ~[2, 4]
    const float invDepth = 1.0f / depth;
    const float sx = centerX + x1 * kFocalLength * invDepth;
    const float sy = centerY - y2 * kFocalLength * invDepth;
    const float radius = 30.0f * invDepth;
    // Depth cue: near LEDs pop, far LEDs recede.
    const float depthDim = std::min(1.15f, std::max(0.45f, 1.65f - 0.33f * depth));

    // Faint socket dot so the wireframe reads even when unlit.
    const int px = (int) sx, py = (int) sy;
    if (px >= 0 && px < _width && py >= 0 && py < _height) {
      float *center = &accum[((size_t) py * _width + px) * 3];
      const float socket = 0.006f * depthDim;
      center[0] += socket;
      center[1] += socket;
      center[2] += socket;
    }

    const ARGB &led = leds[i];
    const float global = std::min<uint8_t>(led.a, 31) / 31.0f;
    const float scale = global * depthDim / 255.0f;
    const float r = scale * led.r;
    const float g = scale * led.g;
    const float b = scale * led.b;
    if (r + g + b < 1e-4f) {
      continue;
    }

    splatAnalytic(accum, _width, _height, sx, sy, radius, r, g, b);
  }
}

void SimRenderer::toneMapRGBA16F(float exposure, float headroom, uint16_t *out) const {
  const float *accum = _accum.data();
  const float invHeadroom = 1.0f / std::max(headroom, 0.01f);
  const size_t pixelCount = (size_t) _width * _height;
  // clang/libc++ only (project policy), so _Float16 is always available.
  _Float16 *dst16 = (_Float16 *) out;
  for (size_t p = 0; p < pixelCount; p++) {
    const float *src = &accum[p * 3];
    _Float16 *dst = &dst16[p * 4];
    dst[0] = (_Float16) toneMapChannel(src[0] * exposure, headroom, invHeadroom);
    dst[1] = (_Float16) toneMapChannel(src[1] * exposure, headroom, invHeadroom);
    dst[2] = (_Float16) toneMapChannel(src[2] * exposure, headroom, invHeadroom);
    dst[3] = (_Float16) 1.0f;
  }
}

void SimRenderer::toneMapSRGB8(float exposure, uint8_t *out) const {
  const float *accum = _accum.data();
  const size_t pixelCount = (size_t) _width * _height;
  for (size_t p = 0; p < pixelCount; p++) {
    const float *src = &accum[p * 3];
    uint8_t *dst = &out[p * 3];
    dst[0] = srgbEncode(toneMapChannel(src[0] * exposure, 1.0f, 1.0f));
    dst[1] = srgbEncode(toneMapChannel(src[1] * exposure, 1.0f, 1.0f));
    dst[2] = srgbEncode(toneMapChannel(src[2] * exposure, 1.0f, 1.0f));
  }
}

std::vector<uint8_t> SimRenderer::renderSRGB8(const ARGB *leds, int ledCount,
                                              const RenderParams &params) {
  composite(leds, ledCount, params.yaw, params.pitch);
  std::vector<uint8_t> out((size_t) _width * _height * 3);
  toneMapSRGB8(params.exposure, out.data());
  return out;
}
