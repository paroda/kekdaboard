add_executable(hello_spi_i2c
  main.c
  master_spi.c
  sd_card.c
  w25qxx.c
  ds3231.c
)

# pull in common dependencies and additional uart hardware support
target_link_libraries(hello_spi_i2c
  pico_stdlib
  # Double tap reset into bootrom is injected by linking with the
  # pico_bootsel_via_double_reset library
  pico_bootsel_via_double_reset
  hardware_i2c hardware_spi
)

# Entering the bootloader in this way also lets us specify a GPIO to be used
# as a bootloader activity LED:
target_compile_definitions(hello_spi_i2c PRIVATE
  PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
)

pico_enable_stdio_usb(hello_spi_i2c 1)
pico_enable_stdio_uart(hello_spi_i2c 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_spi_i2c)
