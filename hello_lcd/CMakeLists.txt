add_executable(hello_lcd
  main.c
  lcd_fonts.c
  lcd_paint.c
  lcd_st7789.c
  master_spi.c
)

# pull in common dependencies and additional uart hardware support
target_link_libraries(hello_lcd pico_stdlib hardware_uart hardware_spi hardware_pwm)

pico_enable_stdio_usb(hello_lcd 1)
pico_enable_stdio_uart(hello_lcd 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_lcd)
