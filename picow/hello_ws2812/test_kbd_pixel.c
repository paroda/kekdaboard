#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pico/stdlib.h>

#include "../kbd/util/led_pixel.h"

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

void draw(uint32_t* pixels, int count) {
    static uint8_t ci = 0;
    uint8_t dc = 0x08;
    uint8_t c = dc + (ci*dc);
    for(uint8_t i=0; i<count; i++) {
        pixels[i] = c << 8;
        c = (c>=0x4f) ? dc : c+dc;
    }
    ci= (ci+1)%8;
}

void draw2(uint32_t* pixels, int count) {
    uint32_t colors[]={0x7f0000, 0x007f00, 0x00007f};
    static int j=0;
    for(int i=0;i<count;i++) {
        pixels[i]= colors[(i+j)%3];
    }
    j=(j+1)%3;
}

void draw3(uint32_t* pixels, int count) {
    static bool on = true;
    for(int i=0; i<count; i++) {
        pixels[i]= on? 0x007f00 : 0;
    }
    on = !on;
}



int main() {
    stdio_init_all();
    printf("Led Pixel Test");

    PIO pio = pio0;
    int sm = 0;
    uint8_t gpio_DI = 7;

    bool use_lib = true;

    led_pixel_t* led;
    uint32_t pixels[LED_COUNT];

    if(use_lib) {
        led = led_pixel_create(pio, sm, gpio_DI, LED_COUNT);
    } else {
        uint8_t offset = pio_add_program(pio, &led_pixel_program);
        led_pixel_program_init(pio, sm, offset, gpio_DI, LED_FREQ);
    }

    uint32_t t0,t,dt;
    t0 = board_millis();
    dt = use_lib ? 500 : 1000;

    while (1) {
        blink_led();

        t = board_millis();
        if(t>t0+dt) {
            t0+=dt;
            if(use_lib) {
                draw3(pixels, LED_COUNT);
                led_pixel_set(led, pixels);
                // led_pixel_set2(led, pixels);
            } else {
                // limit colors to max 127 (0x7f)
                pio_sm_set_pins(pio, sm, 0);
                sleep_us(100);
                pio_sm_put_blocking(pio, sm, 0x7f000000); // green_grb, red_g
                pio_sm_put_blocking(pio, sm, 0x7f000000); // red_rb, blue_gr
                pio_sm_put_blocking(pio, sm, 0x7f007f7f); // blue_b, magenta_grb
                pio_sm_put_blocking(pio, sm, 0x7f007f7f); // cyan_grb, yellow_g
                pio_sm_put_blocking(pio, sm, 0x7f000000); // yellow_rb, padding_gr
            }
        }
        tight_loop_contents();
    }
}
