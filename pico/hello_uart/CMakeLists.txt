add_executable(hello_uart
  hello_uart.c
)

# pull in common dependencies and additional uart hardware support
target_link_libraries(hello_uart pico_stdlib hardware_uart)

pico_enable_stdio_usb(hello_uart 1)
pico_enable_stdio_uart(hello_uart 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_uart)
