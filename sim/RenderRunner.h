#ifndef TREELIGHTS_SIM_RENDERRUNNER_H
#define TREELIGHTS_SIM_RENDERRUNNER_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "sim/render_config.pb.h"

struct RenderInfo {
  int width = 0;
  int height = 0;
  int frameDelayMillis = 16;
  int frameCount = 1;
};

/// Receives each rendered RGB8 frame (width*height*3, valid only during the
/// call). Return false (with *error set) to abort.
using FrameSink =
    std::function<bool(const uint8_t *rgb, std::string *error)>;

/// Deterministically executes a RenderConfig: builds the engine, drives it
/// with the configured audio/time/controls, and streams the requested
/// frame(s) to `sink` — memory stays flat regardless of clip length. `info`
/// is filled before the first sink call. `searchRoots` are tried in order
/// (after the path itself) when resolving wav_path.
bool RunRenderConfigStreaming(const treelights::sim::RenderConfig &config,
                              const std::vector<std::string> &searchRoots,
                              RenderInfo *info, const FrameSink &sink,
                              std::string *error);

struct RenderOutput {
  int width = 0;
  int height = 0;
  int frameDelayMillis = 16;
  // One RGB8 image for stills; N frames for video configs.
  std::vector<std::vector<uint8_t>> frames;
};

/// Convenience wrapper that collects all frames in memory (stills, tests).
bool RunRenderConfig(const treelights::sim::RenderConfig &config,
                     const std::vector<std::string> &searchRoots, RenderOutput *out,
                     std::string *error);

#endif  // TREELIGHTS_SIM_RENDERRUNNER_H
