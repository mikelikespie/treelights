"""Declares rendered images/clips as ordinary build artifacts.

Example:
    sim_render(
        name = "demo_ball",
        config = "configs/ball_video.textproto",
        out = "demo_ball.apng",
    )
Then `bazel build //sim:demo_ball` produces the file under bazel-bin.
"""

def sim_render(name, config, out, data = []):
    native.genrule(
        name = name,
        srcs = [config] + data,
        outs = [out],
        cmd = "$(location //sim:simrender) --config=$(location {config}) --wav_root=$(BINDIR) --out=$@".format(config = config),
        tools = ["//sim:simrender"],
    )
