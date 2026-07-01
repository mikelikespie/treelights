// Snapshot tests: renders every case in testdata/snapshots.textproto and
// compares against the checked-in goldens. Regenerate goldens with:
//   bazel run //sim:simrender -- --update_snapshots

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "google/protobuf/text_format.h"
#include "sim/PngWriter.h"
#include "sim/RenderRunner.h"
#include "sim/render_config.pb.h"

namespace {

constexpr char kSuitePath[] = "sim/testdata/snapshots.textproto";
constexpr char kGoldenDir[] = "sim/testdata/golden";

// Tolerance for libm ULP differences across platforms (libc++ everywhere is
// project policy, so RNG streams match; only transcendental rounding varies).
constexpr int kMaxChannelDelta = 2;
constexpr double kMaxDifferingPixelFraction = 0.0005;

std::string readFileOrDie(const std::string &path) {
  std::ifstream in(path);
  EXPECT_TRUE(in) << "cannot read " << path;
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

void writeActualForDebugging(const std::string &name, const RenderOutput &output) {
  const char *outDir = getenv("TEST_UNDECLARED_OUTPUTS_DIR");
  if (outDir == nullptr) return;
  std::string error;
  WritePng(std::string(outDir) + "/" + name + ".png", output.frames[0].data(),
           output.width, output.height, &error);
}

TEST(SnapshotTest, MatchesGoldens) {
  treelights::sim::SnapshotSuite suite;
  ASSERT_TRUE(
      google::protobuf::TextFormat::ParseFromString(readFileOrDie(kSuitePath), &suite));
  ASSERT_GT(suite.cases_size(), 0);

  for (const auto &snapshotCase : suite.cases()) {
    SCOPED_TRACE(snapshotCase.name());

    RenderOutput output;
    std::string error;
    ASSERT_TRUE(RunRenderConfig(snapshotCase.config(), {"."}, &output, &error)) << error;
    ASSERT_EQ(output.frames.size(), 1u)
        << "snapshot cases must render stills (video_duration_seconds unset)";

    std::vector<uint8_t> golden;
    int goldenW = 0, goldenH = 0;
    std::string goldenPath =
        std::string(kGoldenDir) + "/" + snapshotCase.name() + ".png";
    if (!ReadPng(goldenPath, &golden, &goldenW, &goldenH, &error)) {
      writeActualForDebugging(snapshotCase.name(), output);
      ADD_FAILURE() << "missing/unreadable golden (" << error << "). Regenerate with: "
                    << "bazel run //sim:simrender -- --update_snapshots";
      continue;
    }

    ASSERT_EQ(goldenW, output.width);
    ASSERT_EQ(goldenH, output.height);

    const std::vector<uint8_t> &actual = output.frames[0];
    size_t differingPixels = 0;
    int maxDelta = 0;
    for (size_t p = 0; p < actual.size(); p += 3) {
      int delta = 0;
      for (int c = 0; c < 3; c++) {
        delta = std::max(delta, abs((int) actual[p + c] - (int) golden[p + c]));
      }
      maxDelta = std::max(maxDelta, delta);
      if (delta > kMaxChannelDelta) {
        differingPixels++;
      }
    }

    const size_t pixelCount = actual.size() / 3;
    const double differingFraction = (double) differingPixels / pixelCount;
    if (differingFraction > kMaxDifferingPixelFraction) {
      writeActualForDebugging(snapshotCase.name(), output);
      ADD_FAILURE() << differingPixels << " of " << pixelCount
                    << " pixels differ by more than " << kMaxChannelDelta
                    << " (max channel delta " << maxDelta
                    << "). Actual image written to test outputs. If the change is "
                       "intended: bazel run //sim:simrender -- --update_snapshots";
    }
  }
}

}  // namespace
