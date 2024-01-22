cmake_minimum_required(VERSION 3.5)

# initialize pico-sdk from submodule
# note: this must happen before project()
include(pico-sdk/pico_sdk_init.cmake)

set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk)

project(pico_dipu_kbd C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_DIPU_KBD_PATH ${PROJECT_SOURCE_DIR})

#set(FAMILY rp2040)

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
  -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
  -Wno-unused-function # we have some for the docs that aren't called
  -Wno-maybe-uninitialized
)

# add usb device
add_subdirectory(hid_composite)

# gpio trials
add_subdirectory(hello_gpio_irq)

# add pwm trials
add_subdirectory(hello_pwm)

# add shift_register trials
add_subdirectory(hello_shift_register)

# add uart trials
add_subdirectory(hello_uart)

# add clocks trials
add_subdirectory(hello_clocks)

# add lcd trials
add_subdirectory(hello_lcd)

# add lcd trials
add_subdirectory(hello_track_ball)

# add multiple device trials (flash, sd_card, clock)
add_subdirectory(hello_spi_i2c)

# add ws2812
add_subdirectory(hello_ws2812)

# add kbd
add_subdirectory(kbd)
