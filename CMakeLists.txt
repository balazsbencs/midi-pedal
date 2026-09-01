cmake_minimum_required(VERSION 3.25)

option(MIDI_PEDAL_BUILD_FIRMWARE "Build Pico 2 firmware" OFF)

if(MIDI_PEDAL_BUILD_FIRMWARE)
  include(${CMAKE_CURRENT_SOURCE_DIR}/firmware/pico_sdk_import.cmake)
endif()

project(midi_pedal LANGUAGES C CXX ASM)

if(MIDI_PEDAL_BUILD_FIRMWARE)
  pico_sdk_init()
endif()

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/firmware/CMakeLists.txt")
  add_subdirectory(firmware)
endif()
