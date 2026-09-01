cmake_minimum_required(VERSION 3.25)

option(MIDI_PEDAL_BUILD_FIRMWARE "Build firmware targets" OFF)

if(NOT MIDI_PEDAL_BUILD_FIRMWARE)
  include(${CMAKE_SOURCE_DIR}/cmake/GoogleTest.cmake)
endif()

add_library(midi_core
  src/core/crc32.cpp
  src/core/image_reader.cpp
  src/core/switch_engine.cpp
  src/core/action_engine.cpp
  src/core/expression.cpp
  src/core/config_store.cpp
  src/app/controller.cpp
)
target_include_directories(midi_core PUBLIC src)
target_compile_features(midi_core PUBLIC cxx_std_20)

if(NOT MIDI_PEDAL_BUILD_FIRMWARE)
  add_executable(image_reader_test tests/image_reader_test.cpp)
  target_link_libraries(image_reader_test PRIVATE midi_core GTest::gtest_main)
  target_compile_definitions(image_reader_test PRIVATE MIDI_PEDAL_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

  add_executable(controller_boot_test tests/controller_boot_test.cpp)
  target_link_libraries(controller_boot_test PRIVATE midi_core GTest::gtest_main)

  add_executable(switch_engine_test tests/switch_engine_test.cpp)
  target_link_libraries(switch_engine_test PRIVATE midi_core GTest::gtest_main)

  add_executable(midi_queue_test tests/midi_queue_test.cpp)
  target_link_libraries(midi_queue_test PRIVATE midi_core GTest::gtest_main)

  add_executable(action_engine_test tests/action_engine_test.cpp)
  target_link_libraries(action_engine_test PRIVATE midi_core GTest::gtest_main)

  add_executable(expression_test tests/expression_test.cpp)
  target_link_libraries(expression_test PRIVATE midi_core GTest::gtest_main)

  add_executable(config_store_test tests/config_store_test.cpp)
  target_link_libraries(config_store_test PRIVATE midi_core GTest::gtest_main)
  target_compile_definitions(config_store_test PRIVATE MIDI_PEDAL_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif()

if(MIDI_PEDAL_BUILD_FIRMWARE)
  add_executable(midi_pedal src/main.cpp)
  target_link_libraries(midi_pedal PRIVATE midi_core pico_stdlib hardware_gpio hardware_uart hardware_spi hardware_adc hardware_flash hardware_sync)
  target_include_directories(midi_pedal PRIVATE src)
  target_compile_definitions(midi_pedal PRIVATE PICO_ON_DEVICE=1)
  pico_enable_stdio_usb(midi_pedal 1)
  pico_enable_stdio_uart(midi_pedal 0)
  pico_add_extra_outputs(midi_pedal)
endif()

if(NOT MIDI_PEDAL_BUILD_FIRMWARE)
  include(GoogleTest)
  gtest_discover_tests(image_reader_test)
  gtest_discover_tests(controller_boot_test)
  gtest_discover_tests(switch_engine_test)
  gtest_discover_tests(midi_queue_test)
  gtest_discover_tests(action_engine_test)
  gtest_discover_tests(expression_test)
  gtest_discover_tests(config_store_test)
endif()
