/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define EDGE_FALL 0x4
#define EDGE_RISE 0x8

void gpio_callback(uint gpio, uint32_t events) {
    if (EDGE_FALL == (EDGE_FALL & events)) {
        printf("GPIO %d set to LOW\n\n", gpio);
    } else if (EDGE_RISE == (EDGE_RISE & events)) {
        printf("GPIO %d set to HIGH\n", gpio);
    }
}

int main() {
    stdio_init_all();
    printf("Hello GPIO IRQ\n");

    for (int gpio = 10; gpio <= 15; gpio ++) {
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_set_irq_enabled_with_callback(gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    }

    // Wait forever
    while (1);

    return 0;
}