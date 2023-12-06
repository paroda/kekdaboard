#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pico/stdlib.h>

#include "../kbd/led_pixel.h"

#include "led_pixel.pio.h"
#define LED_FREQ 800000 // 800kHz

#define BLINK 100 
#define LED_COUNT 42


uint32_t board_millis() {
    return us_to_ms(time_us_64());
}

void blink_led() {
    static uint32_t t0 = 0;
    static bool led_on = true;

    if(t0==0) { // init
        t0 = board_millis();
        gpio_init(25);
        gpio_set_dir(25, GPIO_OUT);
        gpio_put(25, led_on);
    } else {
        uint32_t t = board_millis();
        if(t>(t0+BLINK)) {
            led_on = !led_on;
            gpio_put(25, led_on);
            t0 += BLINK;
        }
    }
}

void draw(uint32_t* left, uint32_t* right, int count) {
    static uint32_t c=0, j=0;
    c = (c+1)%128;
    j = (j+(c==0?1:0))%3;
    uint32_t d = 127-c;
    for(int i=0; i<count; i++) {
        left[i] = c << (((i+j)%3)*8);
        right[i] = d << (((i+j)%3)*8);
    }   
}

int main() {
    stdio_init_all();
    printf("Led Pixel Test");

    PIO pio = pio0;
    int sm = 0;
    uint8_t gpio_left = 8;

    bool use_led = true;

    led_pixel_t* led;
    uint32_t left[LED_COUNT];
    uint32_t right[LED_COUNT];

    if(use_led) {
        led = led_pixel_create(pio, sm, gpio_left, LED_COUNT);
    } else {
        uint8_t offset = pio_add_program(pio, &led_pixel_program);
        led_pixel_program_init(pio, sm, offset, gpio_left, LED_FREQ);
    }

    uint32_t t0,t,dt;
    t0 = board_millis();
    dt = use_led ? 30 : 1000;

    while (1) {
        blink_led();

        t = board_millis();
        if(t>t0+dt) {
            t0+=dt;
            if(use_led) {
                draw(left, right, LED_COUNT);
                led_pixel_set(led, left, right);
                // led_pixel_set2(led, left, right);
            } else {
                // limit colors to max 127 (0x7f)
                // 7-1,8-0 = 5, 7-0,8-1 = a, 7-1,8-1 = f
                // 7-red, 8-yellow
                // 7-green, 8-purple
                // 7-blue, 8-cyan
                pio_sm_set_pins(pio, sm, 0);
                sleep_ms(10);
                pio_sm_put_blocking(pio, sm, 0x2aaa3fff); // green,red
                pio_sm_put_blocking(pio, sm, 0x00001555); // blue,green
                pio_sm_put_blocking(pio, sm, 0x2aaa2aaa); // red,blue
                pio_sm_put_blocking(pio, sm, 0x2aaa0000); // green,red
                pio_sm_put_blocking(pio, sm, 0x3fff0000); // blue, padding
            }
        }
        tight_loop_contents();
    }
}