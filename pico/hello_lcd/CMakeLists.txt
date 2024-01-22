add_executable(hello_lcd
  main.c
  ../kbd/master_spi.c
  ../kbd/lcd_st7789.c
  ../kbd/lcd_canvas.c
  ../kbd/lcd_fonts.c
)

# pull in common dependencies and additional uart hardware support
target_link_libraries(hello_lcd
  pico_stdlib
  # Double tap reset into bootrom is injected by linking with the
  # pico_bootsel_via_double_reset library
  pico_bootsel_via_double_reset
  hardware_uart hardware_spi hardware_pwm
)

# Entering the bootloader in this way also lets us specify a GPIO to be used
# as a bootloader activity LED:
target_compile_definitions(hello_lcd PRIVATE
  PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
)

pico_enable_stdio_usb(hello_lcd 1)
pico_enable_stdio_uart(hello_lcd 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_lcd)
