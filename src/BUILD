load(
    "@platformio_rules//platformio:platformio.bzl",
    "platformio_library",
    "platformio_project",
)

cc_library(
    name = "lib_clion",
    srcs = glob([
        "**/*.cc",
        "**/*.cpp",
        "**/*.h",
    ]),
    copts = [
        # Just to make autocomplete happy
        "-I/Applications/Teensyduino.app/Contents/Java/hardware/teensy/avr/cores/teensy4",
        "-I/Applications/Teensyduino.app/Contents/Java/hardware/teensy/avr/libraries/Audio",
        "-I/Applications/Teensyduino.app/Contents/Java/hardware/teensy/avr/libraries/Wire",
        "-I/Applications/Teensyduino.app/Contents/Java/hardware/teensy/avr/libraries/SD/src",
        "-I/Applications/Teensyduino.app/Contents/Java/hardware/teensy/avr/libraries/SerialFlash",
        "-DSEREMU_INTERFACE",
    ],
    visibility = ["//visibility:public"],
    deps = ["//deps/SPI:SPI_clion"],
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
        "SoundHistogramSequence.h",
        "SoundReactiveParticleEffectSequence.h",
        "SoundData.h",
    ],
    add_srcs = [
        "Control.cpp",
        "clock.cpp",
    ],
    hdr = "lib_headers_empty.h",
)

#platformio_project(
#    name = "particles",
#    src = "loop4.cc",
#    board = "teensy40",
#    build_flags = [
#        "-std=c++17",
#        "-mfloat-abi=hard",
#        "-fno-exceptions",
#        "-felide-constructors",
#        "-fno-rtti",
#        "-O3",
#    ],
#    # If you need to you can pass multiple string key-value pairs directly
#    # to PlatformIO. All of these will appear under the environment section in
#    # the generated platformio.ini file. That would be "[env:megaatmega2560]" in
#    # this example. For the available options see:
#    # http://docs.platformio.org/en/stable/projectconf.html
#    environment_kwargs = {
#        "board_build.f_cpu": "600000000L",
#        "build_unflags": "-fsingle-precision-constant",
#    },
#    framework = "arduino",
#    platform = "teensy",
#    deps = [
#        ":lib_headers",
#        "//deps/SPI",
#    ],
#)

platformio_project(
    name = "particles_with_sound",
    src = "particles_with_sound.cc",
    board = "teensy41",
    build_flags = [
        "-std=c++17",
        "-mfloat-abi=hard",
        "-fno-exceptions",
        "-felide-constructors",
        "-fno-rtti",
        "-O3",
    ],
    # If you need to you can pass multiple string key-value pairs directly
    # to PlatformIO. All of these will appear under the environment section in
    # the generated platformio.ini file. That would be "[env:megaatmega2560]" in
    # this example. For the available options see:
    # http://docs.platformio.org/en/stable/projectconf.html
    environment_kwargs = {
        "board_build.f_cpu": "600000000L",
        "build_unflags": "-fsingle-precision-constant",
    },
    framework = "arduino",
    platform = "teensy",
    deps = [
        ":lib_headers",
        "//deps/SPI",
    ],
)
