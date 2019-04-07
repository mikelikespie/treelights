
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)

set(ARDUINO_DEFAULT_BOARD teensy35)

cmake_minimum_required(VERSION 2.8.4)

set(ARDUINO_SDK_PATH /Applications/Arduino.app/Contents/Java)

set(CMAKE_C_COMPILER /Applications/Arduino.app/Contents/Java/hardware/tools/arm/bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER /Applications/Arduino.app/Contents/Java/hardware/tools/arm/bin/arm-none-eabi-g++)

#set(CMAKE_C_COMPILER /usr/local/bin/arm-none-eabi-gcc)
#set(CMAKE_CXX_COMPILER /usr/local/bin/arm-none-eabi-g++)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

set(FCPU 120000000)
set(EXTRA_INCLUDES "-I${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/SPI \
        -I${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/RF24 \
        -I${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/Wire \
        -I${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/EEPROM \
        -I${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/NXPMotionSense")
set(ARDUINO_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -DLAYOUT_US_ENGLISH  ${EXTRA_INCLUDES} -DUSB_SERIAL -iquote libraries/FastLED -Ilibraries/FastLED -DF_CPU=${FCPU} -mfloat-abi=hard  -fno-exceptions -felide-constructors -fno-rtti -O3")
set(ARDUINO_C_FLAGS "${CMAKE_C_FLAGS} -DUSB_SERIAL -DLAYOUT_US_ENGLISH -Ilibraries/FastLED  -iquote libraries/FastLED -DF_CPU=${FCPU} -mfloat-abi=hard -O3")


project(treelights)
register_hardware_platform(${ARDUINO_SDK_PATH}/hardware/teensy/avr)

print_board_list()

message(${TEENSY_BOARDS_PATH})



set(SOURCE_FILES
        src/Control.cpp
        src/ExampleSequence.cpp
        src/clock.cpp
        )

set(HEADER_FILES
        src/Color.h
        src/Context.h
        src/Control.h
        src/ExampleSequence.h
        src/ParticleEffectSequence.h
        src/SequenceBase.h
        src/SinWaveSequence.h
        src/clock.h
        )

set(SPI_INCLUDES ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/SPI)
set(SPI_SRCS     ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/SPI/SPI.cpp)
set(SPI_HDRS     ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/SPI/SPI.h)

generate_arduino_library(SPI
        BOARD teensy35)

set(NXPMotionSense_INCLUDES ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/NXPMotionSense)
set(NXPMotionSense_SRCS
        ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/NXPMotionSense/NXPMotionSense.cpp
        ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/NXPMotionSense/SensorFusion.cpp
        ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/NXPMotionSense/matrix.c
        )
set(NXPMotionSense_HDRS ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/NXPMotionSense/NXPMotionSense.h)
generate_arduino_library(NXPMotionSense
        BOARD teensy35
        LIBS Wire EEPROM)

set(EEPROM_INCLUDES ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/EEPROM)
set(EEPROM_SRCS
        ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/EEPROM/EEPROM.cpp
        )
set(EEPROM_HDRS ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/EEPROM/EEPROM.h)
generate_arduino_library(EEPROM
        BOARD teensy35
        LIBS Wire)

set(RF24_INCLUDES .)
set(RF24_SRCS
        RF24/RF24.cpp
        )
set(RF24_HDRS RF24/RF24.h)
generate_arduino_library(RF24
        BOARD teensy35
        )

set(Wire_INCLUDES ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/Wire)
set(Wire_SRCS
        ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/Wire/Wire.cpp
        ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/Wire/utility/twi.c
        )
set(Wire_HDRS ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/Wire/Wire.h
        ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/Wire/utility/twi.h)

generate_arduino_library(Wire
        BOARD teensy35)

set(treelights_INCLUDES
        libraries/FastLED
        ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/SPI
        ${ARDUINO_SDK_PATH}/hardware/teensy/avr/libraries/Wire
        )


generate_arduino_firmware(treelights
    PROGRAMMER usbtinyisp
    SRCS ${SOURCE_FILES} src/loop2.cpp
    HDRS ${HEADER_FILES}
    BOARD teensy35
    LIBS SPI Wire NXPMotionSense
    )

generate_arduino_firmware(ball
    PROGRAMMER usbtinyisp
    SRCS ${SOURCE_FILES}  src/ballloop.cpp
    HDRS ${HEADER_FILES}
    BOARD teensy35
    LIBS SPI Wire NXPMotionSense RF24
    )
