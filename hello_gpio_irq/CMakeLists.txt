add_executable(hello_gpio_irq
        hello_gpio_irq.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(hello_gpio_irq pico_stdlib)

pico_enable_stdio_usb(hello_gpio_irq 1)
pico_enable_stdio_uart(hello_gpio_irq 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_gpio_irq)
