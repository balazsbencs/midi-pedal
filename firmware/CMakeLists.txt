cmake_minimum_required(VERSION 3.25)

option(MIDI_PEDAL_BUILD_FIRMWARE "Build firmware targets" OFF)

include(${CMAKE_SOURCE_DIR}/cmake/GoogleTest.cmake)

add_library(midi_core
  src/core/crc32.cpp
  src/core/image_reader.cpp
)
target_include_directories(midi_core PUBLIC src)
target_compile_features(midi_core PUBLIC cxx_std_20)

add_executable(image_reader_test tests/image_reader_test.cpp)
target_link_libraries(image_reader_test PRIVATE midi_core GTest::gtest_main)
target_compile_definitions(image_reader_test PRIVATE MIDI_PEDAL_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

include(GoogleTest)
gtest_discover_tests(image_reader_test)
