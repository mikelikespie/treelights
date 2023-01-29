load(
    "@platformio_rules//platformio:platformio.bzl",
    "platformio_library",
    "platformio_project",
)

platformio_library(
    name = "lib_headers",
    src = "lib_headers_empty.cc",
    add_hdrs = [
        "clock.h",
        "Context.h",
        "Color.h",
        "ledmath.h",
        "SequenceBase.h",
        "Control.h",
        "ExampleSequence.h",
        "BurningFlambeosSequence.h",
        "SinWaveSequence.h",
        "ParticleEffectSequence.h",
    ],
    add_srcs = [
        "Control.cpp",
        "clock.cpp",
    ],
    hdr = "lib_headers_empty.h",
)

platformio_project(
    name = "volcano",
    src = "loop4.cc",
    board = "teensy41",
    build_flags = [
        "-std=gnu++17",
        "-mfloat-abi=hard",
        "-fno-exceptions",
        "-felide-constructors",
        "-fno-rtti",
        "-O3",
    ],
    #    build_unflags = [
    #        "-std=gnu++11",
    #        #        build_flags = -std=gnu++2a
    #    ],
    # If you need to you can pass multiple string key-value pairs directly
    # to PlatformIO. All of these will appear under the environment section in
    # the generated platformio.ini file. That would be "[env:megaatmega2560]" in
    # this example. For the available options see:
    # http://docs.platformio.org/en/stable/projectconf.html
    environment_kwargs = {
        "board_f_cpu": "600000000L",
        "build_unflags": "-fsingle-precision-constant",
    },
    framework = "arduino",
    platform = "teensy",
    deps = [
        ":lib_headers",
        "//deps/SPI",
    ],
)
