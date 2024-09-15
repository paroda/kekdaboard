cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk)

set(PICO_PLATFORM rp2040)
set(PICO_BOARD pico_w)

# initialize pico-sdk from submodule
# note: this must happen before project()
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(pico_dipu_kbd C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.0.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.0.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()


add_compile_options(-Wall
  -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
  -Wno-unused-function # we have some for the docs that aren't called
  -Wno-maybe-uninitialized
)

# add projects

add_subdirectory(pico_fota_bootloader)

add_subdirectory(hello_blink)

add_subdirectory(hello_wifi)

add_subdirectory(hello_ws2812)

add_subdirectory(hello_bt)

add_subdirectory(kbd)
