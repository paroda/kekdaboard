#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"

static uint32_t blink_interval_ms = 300;

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
    static uint32_t start_ms = 0;
    static bool led_state = false;

    // Blink every interval ms
    if (board_millis() - start_ms < blink_interval_ms)
        return; // not enough time
    start_ms += blink_interval_ms;

    board_led_write(led_state);

    led_state = 1 - led_state; // toggle
}

void my_task(void) {
    static uint32_t start_ms = 0;

    if(board_millis() - start_ms < 3000) return;

    start_ms += 3000;

    printf("\nHello again from PICO. still beating..");

    uint32_t sys_khz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint32_t peri_khz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    printf("\nclk_sys: %u kHz", sys_khz);
    printf("\nclk_peri: %u kHz", peri_khz);

}

int main(void) {
    stdio_init_all();
    printf("\nHello from PICO. Booting up..");

    while(true) {
        sleep_ms(10);
        led_blinking_task();
        my_task();
    }
}
