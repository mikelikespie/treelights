
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)

set(ARDUINO_DEFAULT_BOARD teensy31)

cmake_minimum_required(VERSION 2.8.4)

set(CMAKE_C_COMPILER /Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-gcc)
set(CMAKE_CXX_COMPILER /Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-g++)

project(treelights)
register_hardware_platform(${ARDUINO_SDK_PATH}/hardware/teensy/)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DTEENSYDUINO=120 -DUSB_SERIAL -Ilibraries/FastLED")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTEENSYDUINO=120 -DUSB_SERIAL -Ilibraries/FastLED")

set(SOURCE_FILES loop.cpp OctoWS2811.cpp)

set(FastLED_SRCS libraries/FastLED/colorpalettes.cpp libraries/FastLED/colorutils.cpp libraries/FastLED/FastLED.cpp libraries/FastLED/hsv2rgb.cpp libraries/FastLED/lib8tion.cpp libraries/FastLED/noise.cpp libraries/FastLED/power_mgt.cpp libraries/FastLED/wiring.cpp)
set(FastLED_HDRS libraries/FastLED/bitswap.h
                 libraries/FastLED/chipsets.h
                 libraries/FastLED/color.h
                 libraries/FastLED/colorpalettes.h
                 libraries/FastLED/colorutils.h
                 libraries/FastLED/controller.h
                 libraries/FastLED/delay.h
                 libraries/FastLED/dmx.h
                 libraries/FastLED/FastLED.h
                 libraries/FastLED/fastled_config.h
                 libraries/FastLED/fastpin.h
                 libraries/FastLED/fastspi.h
                 libraries/FastLED/fastspi_bitbang.h
                 libraries/FastLED/fastspi_dma.h
                 libraries/FastLED/FastSPI_LED2.h
                 libraries/FastLED/fastspi_nop.h
                 libraries/FastLED/fastspi_ref.h
                 libraries/FastLED/fastspi_types.h
                 libraries/FastLED/hsv2rgb.h
                 libraries/FastLED/led_sysdefs.h
                 libraries/FastLED/lib8tion.h
                 libraries/FastLED/noise.h
                 libraries/FastLED/pixeltypes.h
                 libraries/FastLED/platforms/arm/k20/clockless_arm_k20.h
                 libraries/FastLED/platforms/arm/k20/clockless_block_arm_k20.h
                 libraries/FastLED/platforms/arm/k20/fastled_arm_k20.h
                 libraries/FastLED/platforms/arm/k20/fastpin_arm_k20.h
                 libraries/FastLED/platforms/arm/k20/fastspi_arm_k20.h
                 libraries/FastLED/platforms/arm/k20/led_sysdefs_arm_k20.h
                 libraries/FastLED/platforms/arm/k20/octows2811_controller.h
                 libraries/FastLED/platforms/arm/k20/smartmatrix_t3.h
                 libraries/FastLED/platforms/arm/sam/clockless_arm_sam.h
                 libraries/FastLED/platforms/arm/sam/clockless_block_arm_sam.h
                 libraries/FastLED/platforms/arm/sam/fastled_arm_sam.h
                 libraries/FastLED/platforms/arm/sam/fastpin_arm_sam.h
                 libraries/FastLED/platforms/arm/sam/fastspi_arm_sam.h
                 libraries/FastLED/platforms/arm/sam/led_sysdefs_arm_sam.h
                 libraries/FastLED/platforms/avr/clockless_trinket.h
                 libraries/FastLED/platforms/avr/fastled_avr.h
                 libraries/FastLED/platforms/avr/fastpin_avr.h
                 libraries/FastLED/platforms/avr/fastspi_avr.h
                 libraries/FastLED/platforms/avr/led_sysdefs_avr.h
                 libraries/FastLED/platforms.h
                 libraries/FastLED/power_mgt.h
)
generate_arduino_library(FastLED
    BOARD teensy31)

generate_arduino_firmware(treelights
    PROGRAMMER usbtinyisp
    SRCS ${SOURCE_FILES}
    BOARD teensy31
    LIBS FastLED
    NO_AUTOLIBS)