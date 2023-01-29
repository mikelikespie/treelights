set(TEENSY_VERSION 41 CACHE STRING "Set to the Teensy version corresponding to your board (40 or 41 allowed)" FORCE)
set(CPU_CORE_SPEED 600000000 CACHE STRING "Set to 600000000, 24000000, 48000000, 72000000 or 96000000 to set CPU core speed" FORCE) # Derived variables
set(COMPILERPATH "/opt/homebrew/Cellar/arm-none-eabi-gcc/10.3-2021.07/bin")
set(DEPSPATH "../deps")
set(COREPATH "${DEPSPATH}/cores/teensy4/")
find_package(teensy_cmake_macros)
