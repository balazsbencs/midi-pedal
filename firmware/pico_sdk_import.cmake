# Project-local Pico SDK import. The SDK is pinned as the `third_party/pico-sdk` submodule.
set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/../third_party/pico-sdk" CACHE PATH "Pinned Raspberry Pi Pico SDK path")
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
