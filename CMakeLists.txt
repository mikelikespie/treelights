
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)

set(ARDUINO_DEFAULT_BOARD teensy31)

cmake_minimum_required(VERSION 2.8.4)

set(CMAKE_C_COMPILER /Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-gcc)
set(CMAKE_CXX_COMPILER /Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-g++)

project(treelights)
register_hardware_platform(${ARDUINO_SDK_PATH}/hardware/teensy/)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DTEENSYDUINO=120 -DUSB_SERIAL")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTEENSYDUINO=120 -DUSB_SERIAL")

set(SOURCE_FILES loop.cpp OctoWS2811.cpp)

generate_arduino_firmware(treelights
    PROGRAMMER usbtinyisp
    SRCS ${SOURCE_FILES}
    BOARD teensy31
    NO_AUTOLIBS)
