
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

set(ARDUINO_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -DLAYOUT_US_ENGLISH -DUSB_SERIAL -iquote libraries/FastLED -Ilibraries/FastLED -DF_CPU=180000000 -mfloat-abi=hard -I${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/SPI -fno-exceptions -felide-constructors -fno-rtti")
set(ARDUINO_C_FLAGS "${CMAKE_C_FLAGS} -DUSB_SERIAL -DLAYOUT_US_ENGLISH -Ilibraries/FastLED  -iquote libraries/FastLED -DF_CPU=180000000 -mfloat-abi=hard")


project(treelights)
register_hardware_platform(${ARDUINO_SDK_PATH}/hardware/teensy/avr)

print_board_list()

message(${TEENSY_BOARDS_PATH})



set(SOURCE_FILES loop.cpp)

set(SPI_INCLUDES ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/SPI)
set(SPI_SRCS ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/SPI/SPI.cpp)
set(SPI_HDRS ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/SPI/SPI.h)

generate_arduino_library(SPI
        BOARD teensy36)

set(treelights_INCLUDES
        libraries/FastLED
        ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/SPI)

generate_arduino_firmware(treelights
    PROGRAMMER usbtinyisp
    SRCS ${SOURCE_FILES}
    BOARD teensy36
    LIBS SPI
    NO_AUTOLIBS
    )
