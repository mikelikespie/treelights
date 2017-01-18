
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)

set(ARDUINO_DEFAULT_BOARD teensy36)

cmake_minimum_required(VERSION 2.8.4)

set(ARDUINO_SDK_PATH /Applications/Arduino.app/Contents/Java)

set(CMAKE_C_COMPILER /Applications/Arduino.app/Contents/Java/hardware/tools/arm/bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER /Applications/Arduino.app/Contents/Java/hardware/tools/arm/bin/arm-none-eabi-g++)

#set(CMAKE_C_COMPILER /usr/local/bin/arm-none-eabi-gcc)
#set(CMAKE_CXX_COMPILER /usr/local/bin/arm-none-eabi-g++)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

set(ARDUINO_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -DLAYOUT_US_ENGLISH -DUSB_SERIAL -Ilibraries/FastLED -DF_CPU=180000000 -mfloat-abi=hard")
set(ARDUINO_C_FLAGS "${CMAKE_C_FLAGS} -DUSB_SERIAL -DLAYOUT_US_ENGLISH -Ilibraries/FastLED -DF_CPU=180000000 -mfloat-abi=hard")

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections,--relax,--defsym=__rtc_localtime=52711190 -T${ARDUINO_SDK_PATH}/hardware/teensy/avr/cores/teensy3/mk66fx1m0.ld")

project(treelights)
register_hardware_platform(${ARDUINO_SDK_PATH}/hardware/teensy/avr)

print_board_list()

message(${TEENSY_BOARDS_PATH})



set(SOURCE_FILES loop.cpp OctoWS2811.cpp)

set(FastLED_SRCS libraries/FastLED/bitswap.cpp
libraries/FastLED/colorpalettes.cpp
libraries/FastLED/colorutils.cpp
libraries/FastLED/FastLED.cpp
libraries/FastLED/hsv2rgb.cpp
libraries/FastLED/lib8tion.cpp
libraries/FastLED/noise.cpp
libraries/FastLED/power_mgt.cpp
libraries/FastLED/wiring.cpp)
set(FastLED_HDRS libraries/FastLED/bitswap.h
libraries/FastLED/chipsets.h
libraries/FastLED/color.h
libraries/FastLED/colorpalettes.h
libraries/FastLED/colorutils.h
libraries/FastLED/controller.h
libraries/FastLED/cpp_compat.h
libraries/FastLED/dmx.h
libraries/FastLED/FastLED.h
libraries/FastLED/fastled_config.h
libraries/FastLED/fastled_delay.h
libraries/FastLED/fastled_progmem.h
libraries/FastLED/fastpin.h
libraries/FastLED/fastspi.h
libraries/FastLED/fastspi_bitbang.h
libraries/FastLED/fastspi_dma.h
libraries/FastLED/fastspi_nop.h
libraries/FastLED/fastspi_ref.h
libraries/FastLED/fastspi_types.h
libraries/FastLED/hsv2rgb.h
libraries/FastLED/led_sysdefs.h
libraries/FastLED/lib8tion/math8.h
libraries/FastLED/lib8tion/random8.h
libraries/FastLED/lib8tion/scale8.h
libraries/FastLED/lib8tion/trig8.h
libraries/FastLED/lib8tion.h
libraries/FastLED/noise.h
libraries/FastLED/pixelset.h
libraries/FastLED/pixeltypes.h
libraries/FastLED/platforms/arm/common/m0clockless.h
libraries/FastLED/platforms/arm/d21/clockless_arm_d21.h
libraries/FastLED/platforms/arm/d21/fastled_arm_d21.h
libraries/FastLED/platforms/arm/d21/fastpin_arm_d21.h
libraries/FastLED/platforms/arm/d21/led_sysdefs_arm_d21.h
libraries/FastLED/platforms/arm/k20/clockless_arm_k20.h
libraries/FastLED/platforms/arm/k20/clockless_block_arm_k20.h
libraries/FastLED/platforms/arm/k20/fastled_arm_k20.h
libraries/FastLED/platforms/arm/k20/fastpin_arm_k20.h
libraries/FastLED/platforms/arm/k20/fastspi_arm_k20.h
libraries/FastLED/platforms/arm/k20/led_sysdefs_arm_k20.h
libraries/FastLED/platforms/arm/k20/octows2811_controller.h
libraries/FastLED/platforms/arm/k20/smartmatrix_t3.h
libraries/FastLED/platforms/arm/kl26/clockless_arm_kl26.h
libraries/FastLED/platforms/arm/kl26/fastled_arm_kl26.h
libraries/FastLED/platforms/arm/kl26/fastpin_arm_kl26.h
libraries/FastLED/platforms/arm/kl26/fastspi_arm_kl26.h
libraries/FastLED/platforms/arm/kl26/led_sysdefs_arm_kl26.h
libraries/FastLED/platforms/arm/nrf51/clockless_arm_nrf51.h
libraries/FastLED/platforms/arm/nrf51/fastled_arm_nrf51.h
libraries/FastLED/platforms/arm/nrf51/fastpin_arm_nrf51.h
libraries/FastLED/platforms/arm/nrf51/fastspi_arm_nrf51.h
libraries/FastLED/platforms/arm/nrf51/led_sysdefs_arm_nrf51.h
libraries/FastLED/platforms/arm/sam/clockless_arm_sam.h
libraries/FastLED/platforms/arm/sam/clockless_block_arm_sam.h
libraries/FastLED/platforms/arm/sam/fastled_arm_sam.h
libraries/FastLED/platforms/arm/sam/fastpin_arm_sam.h
libraries/FastLED/platforms/arm/sam/fastspi_arm_sam.h
libraries/FastLED/platforms/arm/sam/led_sysdefs_arm_sam.h
libraries/FastLED/platforms/arm/stm32/clockless_arm_stm32.h
libraries/FastLED/platforms/arm/stm32/fastled_arm_stm32.h
libraries/FastLED/platforms/arm/stm32/fastpin_arm_stm32.h
libraries/FastLED/platforms/arm/stm32/led_sysdefs_arm_stm32.h
libraries/FastLED/platforms/avr/clockless_trinket.h
libraries/FastLED/platforms/avr/fastled_avr.h
libraries/FastLED/platforms/avr/fastpin_avr.h
libraries/FastLED/platforms/avr/fastspi_avr.h
libraries/FastLED/platforms/avr/led_sysdefs_avr.h
libraries/FastLED/platforms/esp/8266/clockless_block_esp8266.h
libraries/FastLED/platforms/esp/8266/clockless_esp8266.h
libraries/FastLED/platforms/esp/8266/fastled_esp8266.h
libraries/FastLED/platforms/esp/8266/fastpin_esp8266.h
libraries/FastLED/platforms/esp/8266/led_sysdefs_esp8266.h
libraries/FastLED/platforms.h
libraries/FastLED/power_mgt.h

)
generate_arduino_library(FastLED
    BOARD teensy36)

generate_arduino_firmware(treelights
    PROGRAMMER usbtinyisp
    SRCS ${SOURCE_FILES}
    BOARD teensy36
    LIBS FastLED
    NO_AUTOLIBS)
