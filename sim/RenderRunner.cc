#include "sim/RenderRunner.h"

#include <sys/stat.h>

#include <cmath>

#include "sim/AudioFile.h"
#include "sim/SimEngine.h"
#include "sim/SimRenderer.h"
#include "sim/SyntheticBeat.h"

namespace {

bool fileExists(const std::string &path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}

std::string resolvePath(const std::string &path, const std::vector<std::string> &roots) {
  if (fileExists(path)) {
    return path;
  }
  for (const std::string &root : roots) {
    std::string candidate = root + "/" + path;
    if (fileExists(candidate)) {
      return candidate;
    }
  }
  return "";
}

}  // namespace

bool RunRenderConfig(const treelights::sim::RenderConfig &config,
                     const std::vector<std::string> &searchRoots, RenderOutput *out,
                     std::string *error) {
  using treelights::sim::RenderConfig;

  const SimFixture fixture =
      config.fixture() == RenderConfig::BALL ? SimFixture::kBall : SimFixture::kBars;
  const int stepMillis = config.step_millis() > 0 ? config.step_millis() : 16;
  const float exposure = config.exposure() > 0 ? config.exposure() : 2.2f;
  const float headroom = config.headroom() > 0 ? config.headroom() : 1.0f;
  const float wavGain = config.wav_gain() > 0 ? config.wav_gain() : 4.0f;

  SimEngine engine(fixture);
  if (config.sequence() < 0 || config.sequence() >= engine.sequenceCount()) {
    if (error) *error = "sequence index out of range";
    return false;
  }
  engine.setSequenceIndex(config.sequence());
  for (int i = 0; i < config.controls_size() && i < SimEngine::kMaxControls; i++) {
    engine.setControlValue(i, config.controls(i));
  }

  AudioData wav;
  const bool useWav = config.audio_case() == RenderConfig::kWavPath;
  const bool useSynth = config.audio_case() == RenderConfig::kSyntheticBeat &&
                        config.synthetic_beat();
  if (useWav) {
    std::string resolved = resolvePath(config.wav_path(), searchRoots);
    if (resolved.empty()) {
      if (error) *error = "wav not found: " + config.wav_path();
      return false;
    }
    if (!ReadWav(resolved, &wav, error)) {
      return false;
    }
  }

  SimRenderer renderer(fixture);
  out->width = renderer.width();
  out->height = renderer.height();
  out->frameDelayMillis = stepMillis;
  out->frames.clear();

  const long stillFrame = (long) llround(config.at_seconds() * 1000.0 / stepMillis);
  const long videoFrames =
      (long) llround(config.video_duration_seconds() * 1000.0 / stepMillis);
  const long lastFrame = stillFrame + (videoFrames > 0 ? videoFrames - 1 : 0);

  float bins[SOUND_BUFFER_BIN_COUNT];
  RenderParams params;
  params.exposure = exposure;
  params.headroom = headroom;
  params.pitch = config.pitch();

  for (long frame = 0; frame <= lastFrame; frame++) {
    const double t = frame * stepMillis / 1000.0;
    const float *frameBins = nullptr;
    if (useSynth) {
      SyntheticBeatBins(t, bins);
      frameBins = bins;
    } else if (useWav) {
      WavFftBins512(wav, t, wavGain, bins);
      frameBins = bins;
    }
    engine.tick((uint32_t) (1000 + frame * stepMillis), frameBins);

    if (frame >= stillFrame) {
      params.yaw = config.yaw() + config.yaw_rate() * (float) t;
      out->frames.push_back(
          renderer.renderSRGB8(engine.leds(), engine.totalLedCount(), params));
    }
  }

  if (out->frames.empty()) {
    if (error) *error = "no frames rendered";
    return false;
  }
  return true;
}
