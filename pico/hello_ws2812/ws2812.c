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

#define IS_RGBW false

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

static void pattern_1() {
    for (int i = 0; i < 1000; ++i) {
        switch(i%9) {
        case 0: // red
            put_pixel(ugrb_u32(0x80, 0x00, 0x00));
            break;
        case 1: // green
            put_pixel(ugrb_u32(0x00, 0x80, 0x00));
            break;
        case 2: // blue
            put_pixel(ugrb_u32(0x00, 0x00, 0x80));
            break;
        case 3: // yellow
            put_pixel(ugrb_u32(0x80, 0x80, 0x00));
            break;
        case 4: // purple
            put_pixel(ugrb_u32(0x80, 0x00, 0x80));
            break;
        case 5: // cyan
            put_pixel(ugrb_u32(0x00, 0x80, 0x80));
            break;
        case 6: // off
            put_pixel(ugrb_u32(0x00, 0x00, 0x00));
            break;
        case 7: // half white
            put_pixel(ugrb_u32(0x80, 0x80, 0x80));
            break;
        case 8: // full white
            put_pixel(ugrb_u32(0xff, 0xff, 0xff));
            break;
        default: break;
        }
        if((i+1)%3==0) sleep_ms(1000);
        if((i+1)%9==0) sleep_ms(2000);

        static bool led_on = true;
        gpio_put(25, led_on);
        led_on = !led_on;
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

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    while (1) {
        pattern_1();
        gpio_put(25, led_on);
        led_on = !led_on;
    }
}
