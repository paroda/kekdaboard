/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "ws2812.pio.h"

#define WS2812_PIN 7


static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t ugrb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
        ((uint32_t) (g) << 16) |
        ((uint32_t) (r) << 8) |
        (uint32_t) (b);
}

static void pattern_1(PIO pio, int sm) {
    static int i = 0;
    pio_sm_set_pins(pio, sm, 0);
    sleep_us(60);
    for(int j=0; j<39; j++) {
        switch(i%8) {
        case 0: // red
            put_pixel(ugrb_u32(0x7f, 0x00, 0x00));
            break;
        case 1: // green
            put_pixel(ugrb_u32(0x00, 0x7f, 0x00));
            break;
        case 2: // blue
            put_pixel(ugrb_u32(0x00, 0x00, 0x7f));
            break;
        case 3: // yellow
            put_pixel(ugrb_u32(0x7f, 0x7f, 0x00));
            break;
        case 4: // magenta
            put_pixel(ugrb_u32(0x7f, 0x00, 0x7f));
            break;
        case 5: // cyan
            put_pixel(ugrb_u32(0x00, 0x7f, 0x7f));
            break;
        case 6: // off
            put_pixel(ugrb_u32(0x00, 0x00, 0x00));
            break;
        case 7: // half white
            put_pixel(ugrb_u32(0x7f, 0x7f, 0x7f));
            break;
        default: break;
        }
        i++;
    }
}

int main() {
    stdio_init_all();
    printf("WS2812 Smoke Test, using pin %d", WS2812_PIN);

    // init board led
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    bool led_on = true;
    gpio_put(25, led_on);

    // todo get free sm
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);


    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000);

    int i = 0;
    while (1) {
        // printf("hello");
        if(i++%10==0) pattern_1(pio,sm);
        // blink led
        sleep_ms(500);
        gpio_put(25, led_on);
        led_on = !led_on;
    }
}
