cmake_minimum_required(VERSION 3.25)

project(midi_pedal LANGUAGES CXX)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/firmware/CMakeLists.txt")
  add_subdirectory(firmware)
endif()
