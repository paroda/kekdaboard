add_executable(picow_blink
  picow_blink.c
)

target_link_libraries(picow_blink
  pico_stdlib              # for core functionality
  pico_cyw43_arch_none     # we need Wifi to access the GPIO, but we don't need anything else
)

pico_enable_stdio_usb(picow_blink 1)
pico_enable_stdio_uart(picow_blink 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(picow_blink)
