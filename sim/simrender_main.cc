// Offline renderer: RenderConfig/SnapshotSuite textprotos -> PNG stills or
// APNG clips. See sim/render_config.proto for the schema.
//
// Usage:
//   simrender --config=cfg.textproto --out=shot.png [--wav_root=DIR]
//   simrender --suite=suite.textproto --outdir=DIR
//   simrender --update_snapshots      (regenerates sim/testdata/golden/*)

#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "google/protobuf/text_format.h"
#include "sim/PngWriter.h"
#include "sim/RenderRunner.h"
#include "sim/render_config.pb.h"

namespace {

std::string dirname(const std::string &path) {
  size_t slash = path.find_last_of('/');
  return slash == std::string::npos ? std::string(".") : path.substr(0, slash);
}

void makeDirs(const std::string &path) {
  std::string partial;
  for (size_t i = 0; i < path.size(); i++) {
    if (path[i] == '/' && !partial.empty()) {
      mkdir(partial.c_str(), 0755);
    }
    partial.push_back(path[i]);
  }
  if (!partial.empty()) {
    mkdir(partial.c_str(), 0755);
  }
}

bool readFile(const std::string &path, std::string *out) {
  std::ifstream in(path);
  if (!in) return false;
  std::stringstream ss;
  ss << in.rdbuf();
  *out = ss.str();
  return true;
}

template <typename Proto>
bool parseTextproto(const std::string &path, Proto *proto) {
  std::string text;
  if (!readFile(path, &text)) {
    fprintf(stderr, "cannot read %s\n", path.c_str());
    return false;
  }
  if (!google::protobuf::TextFormat::ParseFromString(text, proto)) {
    fprintf(stderr, "cannot parse %s as %s\n", path.c_str(),
            std::string(Proto::descriptor()->full_name()).c_str());
    return false;
  }
  return true;
}

/// Streams frames straight into the PNG/APNG writer so long clips never
/// accumulate in memory.
bool renderToFile(const treelights::sim::RenderConfig &config,
                  const std::vector<std::string> &roots, const std::string &outPath) {
  RenderInfo info;
  ApngWriter apng;
  bool apngStarted = false;
  std::vector<uint8_t> still;
  std::string error;

  bool ok = RunRenderConfigStreaming(
      config, roots, &info,
      [&](const uint8_t *rgb, std::string *sinkError) {
        if (info.frameCount == 1) {
          still.assign(rgb, rgb + (size_t) info.width * info.height * 3);
          return true;
        }
        if (!apngStarted) {
          if (!apng.begin(outPath, info.width, info.height, info.frameCount,
                          info.frameDelayMillis, sinkError)) {
            return false;
          }
          apngStarted = true;
        }
        return apng.addFrame(rgb, sinkError);
      },
      &error);
  if (!ok) {
    fprintf(stderr, "render failed: %s\n", error.c_str());
    return false;
  }

  ok = info.frameCount == 1
           ? WritePng(outPath, still.data(), info.width, info.height, &error)
           : apng.finish(&error);
  if (!ok) {
    fprintf(stderr, "write failed: %s\n", error.c_str());
    return false;
  }
  printf("%s: %dx%d, %d frame(s)\n", outPath.c_str(), info.width, info.height,
         info.frameCount);
  return true;
}

bool fileExists(const std::string &path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}

/// `bazel run` chdirs into the runfiles tree; resolve user-supplied paths
/// against the invocation directory (BUILD_WORKING_DIRECTORY) as well.
std::string resolveInputPath(const std::string &path) {
  if (path.empty() || path[0] == '/' || fileExists(path)) {
    return path;
  }
  const char *workingDir = getenv("BUILD_WORKING_DIRECTORY");
  if (workingDir != nullptr) {
    std::string candidate = std::string(workingDir) + "/" + path;
    if (fileExists(candidate)) {
      return candidate;
    }
  }
  return path;
}

std::string resolveOutputPath(const std::string &path) {
  if (path.empty() || path[0] == '/') {
    return path;
  }
  const char *workingDir = getenv("BUILD_WORKING_DIRECTORY");
  return workingDir != nullptr ? std::string(workingDir) + "/" + path : path;
}

}  // namespace

int main(int argc, char **argv) {
  std::string configPath, suitePath, outPath, outDir, wavRoot;
  bool updateSnapshots = false;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    auto value = [&](const char *flag) -> const char * {
      size_t len = strlen(flag);
      return arg.compare(0, len, flag) == 0 ? arg.c_str() + len : nullptr;
    };
    if (const char *v = value("--config=")) {
      configPath = v;
    } else if (const char *v = value("--suite=")) {
      suitePath = v;
    } else if (const char *v = value("--out=")) {
      outPath = v;
    } else if (const char *v = value("--outdir=")) {
      outDir = v;
    } else if (const char *v = value("--wav_root=")) {
      wavRoot = v;
    } else if (arg == "--update_snapshots") {
      updateSnapshots = true;
    } else {
      fprintf(stderr, "unknown flag: %s\n", arg.c_str());
      return 2;
    }
  }

  if (updateSnapshots) {
    const char *workspace = getenv("BUILD_WORKSPACE_DIRECTORY");
    if (workspace == nullptr) {
      fprintf(stderr, "--update_snapshots must run via `bazel run //sim:simrender`\n");
      return 2;
    }
    suitePath = std::string(workspace) + "/sim/testdata/snapshots.textproto";
    outDir = std::string(workspace) + "/sim/testdata/golden";
  } else {
    configPath = resolveInputPath(configPath);
    suitePath = resolveInputPath(suitePath);
    outPath = resolveOutputPath(outPath);
    outDir = resolveOutputPath(outDir);
  }

  if (!suitePath.empty()) {
    if (outDir.empty()) {
      fprintf(stderr, "--suite requires --outdir\n");
      return 2;
    }
    treelights::sim::SnapshotSuite suite;
    if (!parseTextproto(suitePath, &suite)) return 1;
    makeDirs(outDir);
    std::vector<std::string> roots = {dirname(suitePath)};
    if (!wavRoot.empty()) roots.push_back(wavRoot);
    if (const char *wd = getenv("BUILD_WORKING_DIRECTORY")) roots.push_back(wd);
    for (const auto &snapshotCase : suite.cases()) {
      if (!renderToFile(snapshotCase.config(), roots,
                        outDir + "/" + snapshotCase.name() + ".png")) {
        return 1;
      }
    }
    return 0;
  }

  if (configPath.empty() || outPath.empty()) {
    fprintf(stderr,
            "usage: simrender --config=cfg.textproto --out=img.png [--wav_root=DIR]\n"
            "       simrender --suite=suite.textproto --outdir=DIR\n"
            "       simrender --update_snapshots\n");
    return 2;
  }

  treelights::sim::RenderConfig config;
  if (!parseTextproto(configPath, &config)) return 1;
  std::vector<std::string> roots = {dirname(configPath)};
  if (!wavRoot.empty()) roots.push_back(wavRoot);
  if (const char *wd = getenv("BUILD_WORKING_DIRECTORY")) roots.push_back(wd);
  return renderToFile(config, roots, outPath) ? 0 : 1;
}
