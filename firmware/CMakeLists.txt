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
  src/core/frame_decoder.cpp
  src/display/live_renderer.cpp
  src/display/font_ascii.cpp
  src/usb/usb_descriptors.c
  src/usb/usb_transport.cpp
  src/usb/protocol_dispatcher.cpp
  src/usb/pico_usb.cpp
  src/app/controller.cpp
  src/app/controller_protocol_service.cpp
  src/app/pedal_runtime.cpp
)
set_source_files_properties(src/usb/usb_descriptors.c PROPERTIES LANGUAGE CXX)
target_include_directories(midi_core PUBLIC src)
target_compile_features(midi_core PUBLIC cxx_std_20)

add_library(pico_hal
  src/hal/pico_expression_adc.cpp
  src/hal/pico_flash_port.cpp
  src/hal/pico_relays.cpp
  src/hal/pico_switches.cpp
  src/hal/pico_uart_midi.cpp
  src/hal/pico_watchdog.cpp
  src/display/st7796s.cpp
)
target_include_directories(pico_hal PUBLIC src)
target_link_libraries(pico_hal PUBLIC midi_core)
target_compile_features(pico_hal PUBLIC cxx_std_20)

if(NOT MIDI_PEDAL_BUILD_FIRMWARE)
  add_executable(image_reader_test tests/image_reader_test.cpp)
  target_link_libraries(image_reader_test PRIVATE midi_core GTest::gtest_main)
  target_compile_definitions(image_reader_test PRIVATE MIDI_PEDAL_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

  add_executable(controller_boot_test tests/controller_boot_test.cpp)
  target_link_libraries(controller_boot_test PRIVATE midi_core GTest::gtest_main)

  add_executable(controller_protocol_service_test tests/controller_protocol_service_test.cpp)
  target_link_libraries(controller_protocol_service_test PRIVATE midi_core GTest::gtest_main)
  target_compile_definitions(controller_protocol_service_test PRIVATE MIDI_PEDAL_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

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

  add_executable(frame_decoder_test tests/frame_decoder_test.cpp)
  target_link_libraries(frame_decoder_test PRIVATE midi_core GTest::gtest_main)

  add_executable(usb_descriptor_test tests/usb_descriptor_test.cpp)
  target_link_libraries(usb_descriptor_test PRIVATE midi_core GTest::gtest_main)

  add_executable(usb_transport_test tests/usb_transport_test.cpp)
  target_link_libraries(usb_transport_test PRIVATE midi_core GTest::gtest_main)

  add_executable(pico_usb_port_test tests/pico_usb_port_test.cpp)
  target_link_libraries(pico_usb_port_test PRIVATE midi_core GTest::gtest_main)

  add_executable(protocol_dispatcher_test tests/protocol_dispatcher_test.cpp)
  target_link_libraries(protocol_dispatcher_test PRIVATE midi_core GTest::gtest_main)

  add_executable(pedal_runtime_test tests/pedal_runtime_test.cpp)
  target_link_libraries(pedal_runtime_test PRIVATE midi_core GTest::gtest_main)

  add_executable(pico_hal_contract_test tests/pico_hal_contract_test.cpp)
  target_link_libraries(pico_hal_contract_test PRIVATE pico_hal GTest::gtest_main)

  add_executable(live_renderer_test tests/live_renderer_test.cpp)
  target_link_libraries(live_renderer_test PRIVATE midi_core GTest::gtest_main)
  target_compile_definitions(live_renderer_test PRIVATE MIDI_PEDAL_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

  add_executable(display_golden_generator tests/display_golden_generator.cpp)
  target_link_libraries(display_golden_generator PRIVATE midi_core)
  target_compile_definitions(display_golden_generator PRIVATE MIDI_PEDAL_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif()

if(MIDI_PEDAL_BUILD_FIRMWARE)
  set(FACTORY_IMAGE_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/factory_empty_image.o)
  add_custom_command(
    OUTPUT ${FACTORY_IMAGE_OBJECT}
    COMMAND ${CMAKE_OBJCOPY} -I binary -O elf32-littlearm -B arm
            --rename-section .data=.rodata,alloc,load,readonly,data,contents
            firmware/defaults/factory-empty.bin ${FACTORY_IMAGE_OBJECT}
    DEPENDS ${CMAKE_SOURCE_DIR}/firmware/defaults/factory-empty.bin
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
  )
  add_executable(midi_pedal src/main.cpp src/usb/pico_usb_device.cpp ${FACTORY_IMAGE_OBJECT})
  target_link_libraries(midi_pedal PRIVATE pico_hal midi_core tinyusb_device pico_stdlib pico_unique_id hardware_gpio hardware_uart hardware_spi hardware_adc hardware_flash hardware_sync hardware_dma hardware_watchdog)
  target_include_directories(midi_pedal PRIVATE src)
  target_compile_definitions(midi_pedal PRIVATE PICO_ON_DEVICE=1)
  pico_enable_stdio_usb(midi_pedal 0)
  pico_enable_stdio_uart(midi_pedal 0)
  pico_add_extra_outputs(midi_pedal)
endif()

if(NOT MIDI_PEDAL_BUILD_FIRMWARE)
  include(GoogleTest)
  gtest_discover_tests(image_reader_test)
  gtest_discover_tests(controller_boot_test)
  gtest_discover_tests(controller_protocol_service_test)
  gtest_discover_tests(switch_engine_test)
  gtest_discover_tests(midi_queue_test)
  gtest_discover_tests(action_engine_test)
  gtest_discover_tests(expression_test)
  gtest_discover_tests(config_store_test)
  gtest_discover_tests(frame_decoder_test)
  gtest_discover_tests(usb_descriptor_test)
  gtest_discover_tests(usb_transport_test)
  gtest_discover_tests(pico_usb_port_test)
  gtest_discover_tests(protocol_dispatcher_test)
  gtest_discover_tests(pedal_runtime_test)
  gtest_discover_tests(pico_hal_contract_test)
  gtest_discover_tests(live_renderer_test)
endif()
