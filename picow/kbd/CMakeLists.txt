cmake_minimum_required(VERSION 3.5)

############################################################

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)


############################################################
## Access point

add_executable(kbd_ap
  ap/main.c
  ap/dhcp_server.c

  core0.c
  core1.c
  data_model.c
  hw_model.c
  input_processor.c
  key_layout.c
  usb_hid.c

  screen_model.c
  screen/date.c
  screen/pixel.c
  screen/power.c
  screen/scan.c
  screen/tb.c
  screen/welcome.c

  util/shared_buffer.c
  util/master_spi.c
  util/flash_store.c
  util/flash_w25qxx.c
  util/usb_descriptors.c
)

target_include_directories(kbd_ap PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/ap
)

target_link_libraries(kbd_ap
  pico_fota_bootloader_lib
  pico_cyw43_arch_lwip_poll
  pico_stdlib
  pico_multicore
  hardware_spi
  tinyusb_device
  tinyusb_board
)

target_link_options(kbd_ap PRIVATE
  "-L${CMAKE_CURRENT_SOURCE_DIR}/../pico_fota_bootloader/linker_common"
)

pico_set_linker_script(kbd_ap
  ${CMAKE_CURRENT_SOURCE_DIR}/../pico_fota_bootloader/linker_common/application.ld
)

target_compile_definitions(kbd_ap PRIVATE
)

pico_enable_stdio_usb(kbd_ap 0)
pico_enable_stdio_uart(kbd_ap 0)

pico_add_extra_outputs(kbd_ap)


############################################################
## Left node

add_executable(kbd_left
  left/main.c

  core0.c
  core1.c
  data_model.c
  hw_model.c
  key_layout.c

  screen_model.c
  screen/date.c
  screen/pixel.c
  screen/power.c
  screen/scan.c
  screen/tb.c
  screen/welcome.c

  util/shared_buffer.c
  util/master_spi.c
  util/lcd_canvas.c
  util/lcd_fonts.c
  util/lcd_st7789.c
  util/led_pixel.c
  util/rtc_ds3231.c
  util/key_scan.c
  util/pixel_anim.c
)

# generate the header file into the source tree
pico_generate_pio_header(kbd_left
  ${CMAKE_CURRENT_LIST_DIR}/util/led_pixel.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated
)

target_include_directories(kbd_left PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/left
)

target_link_libraries(kbd_left
  pico_fota_bootloader_lib
  pico_cyw43_arch_lwip_poll
  pico_stdlib
  pico_multicore
  hardware_spi
  hardware_i2c
  hardware_pwm
  hardware_pio
  hardware_dma
)

target_link_options(kbd_left PRIVATE
  "-L${CMAKE_CURRENT_SOURCE_DIR}/../pico_fota_bootloader/linker_common"
)

pico_set_linker_script(kbd_left
  ${CMAKE_CURRENT_SOURCE_DIR}/../pico_fota_bootloader/linker_common/application.ld
)

target_compile_definitions(kbd_left PRIVATE
)

pico_enable_stdio_usb(kbd_left 0)
pico_enable_stdio_uart(kbd_left 0)

pico_add_extra_outputs(kbd_left)


############################################################
## Right node

add_executable(kbd_right
  right/main.c

  core0.c
  core1.c
  data_model.c
  hw_model.c
  key_layout.c

  screen_model.c
  screen/date.c
  screen/pixel.c
  screen/power.c
  screen/scan.c
  screen/tb.c
  screen/welcome.c

  util/shared_buffer.c
  util/master_spi.c
  util/led_pixel.c
  util/srom_pmw3389.c
  util/tb_pmw3389.c
  util/key_scan.c
  util/pixel_anim.c
)

# generate the header file into the source tree
pico_generate_pio_header(kbd_right
  ${CMAKE_CURRENT_LIST_DIR}/util/led_pixel.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated
)

target_include_directories(kbd_right PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/right
)

target_link_libraries(kbd_right
  pico_fota_bootloader_lib
  pico_cyw43_arch_lwip_poll
  pico_stdlib
  pico_multicore
  hardware_spi
  hardware_pio
  hardware_dma
)

target_link_options(kbd_right PRIVATE
  "-L${CMAKE_CURRENT_SOURCE_DIR}/../pico_fota_bootloader/linker_common"
)

pico_set_linker_script(kbd_right
  ${CMAKE_CURRENT_SOURCE_DIR}/../pico_fota_bootloader/linker_common/application.ld
)

target_compile_definitions(kbd_right PRIVATE
)

pico_enable_stdio_usb(kbd_right 0)
pico_enable_stdio_uart(kbd_right 0)

pico_add_extra_outputs(kbd_right)
