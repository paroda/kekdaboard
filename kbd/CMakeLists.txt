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

set(FAMILY rp2040)
set(BOARD pico_sdk)

add_executable(kbd_main
  main.c
  data_model.c
  flash_w25qxx.c
  input_processor.c
  key_layout.c
  key_scan.c
  lcd_canvas.c
  lcd_fonts.c
  lcd_st7789.c
  master_spi.c
  peer_comm.c
  rtc_ds3231.c
  shared_buffer.c
  srom_pmw3389.c
  tb_pmw3389.c
  uart_comm.c
  usb_descriptors.c
  usb_hid.c
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

target_compile_definitions(flash_right PRIVATE
  PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
)

pico_enable_stdio_usb(kbd_main 0)
pico_enable_stdio_uart(kbd_main 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(kbd_main)
