#ifndef __BLINK_LED_H
#define __BLINK_LED_H

#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"

static uint32_t blink_interval_ms = 100;

static uint32_t board_millis(void) {
    return to_ms_since_boot(get_absolute_time());
}

static void board_led_write(bool state) {
    static bool inited = false;

    if(!inited) {
        gpio_init(25);
        gpio_set_dir(25, GPIO_OUT);
        inited = true;
    }

    gpio_put(25, state);
}

static void led_blinking_task(void) {
    static bool led_state = false;
    static uint32_t start_ms = 0;
    if(start_ms == 0) start_ms = board_millis();

    // Blink every interval ms
    if (board_millis() - start_ms < blink_interval_ms)
        return; // not enough time
    start_ms += blink_interval_ms;

    board_led_write(led_state);

    led_state = 1 - led_state; // toggle
}

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
