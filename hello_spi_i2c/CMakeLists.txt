add_executable(hello_spi_i2c
  main.c
  master_spi.c
  flash_w25qxx.c
  rtc_ds3231.c
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


#-----------------------

# example sd_card
add_subdirectory(sd_card build)

add_executable(ex_sd
  sd_card/sd_example.c
  master_spi.c
  rtc_ds3231.c
)

target_link_libraries(ex_sd
  FatFs_SPI
  pico_bootsel_via_double_reset
  hardware_i2c
)

# Entering the bootloader in this way also lets us specify a GPIO to be used
# as a bootloader activity LED:
target_compile_definitions(ex_sd PRIVATE
  PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
)

pico_enable_stdio_usb(ex_sd 1)
pico_enable_stdio_uart(ex_sd 0)

pico_add_extra_outputs(ex_sd)
