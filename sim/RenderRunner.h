#ifndef TREELIGHTS_SIM_RENDERRUNNER_H
#define TREELIGHTS_SIM_RENDERRUNNER_H

#include <cstdint>
#include <string>
#include <vector>

#include "sim/render_config.pb.h"

struct RenderOutput {
  int width = 0;
  int height = 0;
  int frameDelayMillis = 16;
  // One RGB8 image for stills; N frames for video configs.
  std::vector<std::vector<uint8_t>> frames;
};

/// Deterministically executes a RenderConfig: builds the engine, drives it
/// with the configured audio/time/controls, and composites the requested
/// frame(s). `searchRoots` are tried in order (after the path itself) when
/// resolving wav_path.
bool RunRenderConfig(const treelights::sim::RenderConfig &config,
                     const std::vector<std::string> &searchRoots, RenderOutput *out,
                     std::string *error);

#endif  // TREELIGHTS_SIM_RENDERRUNNER_H
