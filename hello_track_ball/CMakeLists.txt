add_executable(hello_track_ball
  main.c
  master_spi.c
  srom_pmw3389.c
  tb_pmw3389.c
)

# pull in common dependencies and additional uart hardware support
target_link_libraries(hello_track_ball
  pico_stdlib
  # Double tap reset into bootrom is injected by linking with the
  # pico_bootsel_via_double_reset library
  pico_bootsel_via_double_reset
  hardware_uart hardware_spi hardware_pwm
)

# Entering the bootloader in this way also lets us specify a GPIO to be used
# as a bootloader activity LED:
target_compile_definitions(hello_track_ball PRIVATE
  PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
)

pico_enable_stdio_usb(hello_track_ball 1)
pico_enable_stdio_uart(hello_track_ball 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_track_ball)
