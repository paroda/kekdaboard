cmake_minimum_required(VERSION 3.5)

############################################################
# flash_left
add_executable(flash_left
  main_flash_side.c
  side_left.c
  master_spi.c
  flash_w25qxx.c
)

target_link_libraries(flash_left
  pico_stdlib
  pico_bootsel_via_double_reset # double tap reset into bootrom
  hardware_spi hardware_i2c hardware_uart
)

target_compile_definitions(flash_left PRIVATE
  PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
)

pico_enable_stdio_usb(flash_left 1)
pico_enable_stdio_uart(flash_left 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(flash_left)

############################################################
# flash_right
add_executable(flash_right
  main_flash_side.c
  side_right.c
  master_spi.c
  flash_w25qxx.c
)

target_link_libraries(flash_right
  pico_stdlib
  pico_bootsel_via_double_reset # Double tap reset into bootrom
  hardware_spi hardware_i2c hardware_uart
)

target_compile_definitions(flash_right PRIVATE
  PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
)

pico_enable_stdio_usb(flash_right 1)
pico_enable_stdio_uart(flash_right 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(flash_right)

############################################################
# main

project(kbd_main)

add_executable(kbd_main)

target_sources(kbd_main PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/main.c
  ${CMAKE_CURRENT_LIST_DIR}/data_model.c
  ${CMAKE_CURRENT_LIST_DIR}/flash_w25qxx.c
  ${CMAKE_CURRENT_LIST_DIR}/input_processor.c
  ${CMAKE_CURRENT_LIST_DIR}/key_layout.c
  ${CMAKE_CURRENT_LIST_DIR}/key_scan.c
  ${CMAKE_CURRENT_LIST_DIR}/lcd_canvas.c
  ${CMAKE_CURRENT_LIST_DIR}/lcd_fonts.c
  ${CMAKE_CURRENT_LIST_DIR}/lcd_st7789.c
  ${CMAKE_CURRENT_LIST_DIR}/master_spi.c
  ${CMAKE_CURRENT_LIST_DIR}/peer_comm.c
  ${CMAKE_CURRENT_LIST_DIR}/rtc_ds3231.c
  ${CMAKE_CURRENT_LIST_DIR}/screen_date.c
  ${CMAKE_CURRENT_LIST_DIR}/screen_processor.c
  ${CMAKE_CURRENT_LIST_DIR}/screen_scan.c
  ${CMAKE_CURRENT_LIST_DIR}/screen_welcome.c
  ${CMAKE_CURRENT_LIST_DIR}/shared_buffer.c
  ${CMAKE_CURRENT_LIST_DIR}/srom_pmw3389.c
  ${CMAKE_CURRENT_LIST_DIR}/tb_pmw3389.c
  ${CMAKE_CURRENT_LIST_DIR}/uart_comm.c
  ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
  ${CMAKE_CURRENT_LIST_DIR}/usb_hid.c
)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(kbd_main PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(kbd_main PUBLIC
  pico_stdlib
  pico_multicore
  pico_bootsel_via_double_reset # Double tap reset into bootrom
  hardware_spi hardware_i2c hardware_uart hardware_pwm
  tinyusb_device tinyusb_board
)

target_compile_definitions(kbd_main PRIVATE
  PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
)

pico_enable_stdio_usb(kbd_main 0)
pico_enable_stdio_uart(kbd_main 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(kbd_main)
