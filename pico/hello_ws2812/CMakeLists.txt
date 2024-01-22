add_executable(hello_ws2812
  ws2812.c
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)

# generate the header file into the source tree as it is included in the RP2040 datasheet
pico_generate_pio_header(hello_ws2812
  ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated
)

target_link_libraries(hello_ws2812 PRIVATE
  pico_stdlib hardware_pio
  # Double tap reset into bootrom is injected by linking with the
  # pico_bootsel_via_double_reset library
  pico_bootsel_via_double_reset
)

# Entering the bootloader in this way also lets us specify a GPIO to be used
# as a bootloader activity LED:
target_compile_definitions(hello_ws2812 PRIVATE
  PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
)

pico_enable_stdio_usb(hello_ws2812 1)
pico_enable_stdio_uart(hello_ws2812 0)

pico_add_extra_outputs(hello_ws2812)

################################################################################

add_executable(test_kbd_pixel
  test_kbd_pixel.c
  ../kbd/led_pixel.c
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../kbd/generated)

# generate the header file into the source tree as it is included in the RP2040 datasheet
pico_generate_pio_header(test_kbd_pixel
  ${CMAKE_CURRENT_LIST_DIR}/../kbd/led_pixel.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/../kbd/generated
)

target_link_libraries(test_kbd_pixel PRIVATE
  pico_stdlib hardware_pio hardware_dma
  # Double tap reset into bootrom is injected by linking with the
  # pico_bootsel_via_double_reset library
  pico_bootsel_via_double_reset
)

# Entering the bootloader in this way also lets us specify a GPIO to be used
# as a bootloader activity LED:
target_compile_definitions(test_kbd_pixel PRIVATE
  PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
)

pico_enable_stdio_usb(test_kbd_pixel 1)
pico_enable_stdio_uart(test_kbd_pixel 0)

pico_add_extra_outputs(test_kbd_pixel)
