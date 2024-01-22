add_executable(hello_clocks
  main.c
)

# pull in common dependencies and additional uart hardware support
target_link_libraries(hello_clocks pico_stdlib hardware_clocks)

pico_enable_stdio_usb(hello_clocks 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_clocks)
