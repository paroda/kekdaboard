cmake_minimum_required(VERSION 3.5)

project(hid_composite)

add_executable(hid_composite)

target_sources(hid_composite PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c
)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(hid_composite PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(hid_composite PUBLIC
  pico_stdlib
  pico_bootsel_via_double_reset # Double tap reset into bootrom
  tinyusb_device tinyusb_board
)

target_compile_definitions(hid_composite PRIVATE
  PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(hid_composite)
