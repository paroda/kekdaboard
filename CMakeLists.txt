cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_dipu_kbd C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_DIPU_KBD_PATH ${PROJECT_SOURCE_DIR})

set(FAMILY rp2040)

# Initialize the SDK
pico_sdk_init()



add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

# gpio trials
add_subdirectory(hello_gpio_irq)

# add usb device
add_subdirectory(hid_composite)