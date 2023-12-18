#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "led_pixel.pio.h"
#include "led_pixel.h"

#define LED_FREQ 800000 // 800kHz

led_pixel_t* led_pixel_create(pio_hw_t* pio, uint8_t sm, uint8_t gpio_left_DI, uint8_t count) {
    led_pixel_t* led = (led_pixel_t*) malloc(sizeof(led_pixel_t));
    led->pio = pio;
    led->sm = sm;
    led->gpio_left_DI = gpio_left_DI;
    led->count = count;
    led->on = true;
    // 2 sides, each with `count` pixels, each 3 bytes of color (rgb)
    // one word (32 bit) stores two color components of each side.
    // hence (2*3*N/4) as many words needed.
    // And add one more to handle round down odd number of pixels.
    uint32_t n = 6 * (uint32_t) led->count;
    led->buff_size = n/4 + (n%4 ? 1 : 0);
    led->buff = (uint32_t*) malloc(led->buff_size*4);
    memset(led->buff, 0, 4*led->buff_size);

    led->chan = dma_claim_unused_channel(false);
    if(led->chan >= 0)
    {
        led->chan_config = dma_channel_get_default_config(led->chan);
        channel_config_set_dreq(&led->chan_config, pio_get_dreq(led->pio, led->sm, true));
        channel_config_set_read_increment(&led->chan_config, true);
        channel_config_set_write_increment(&led->chan_config, false);
        channel_config_set_transfer_data_size(&led->chan_config, DMA_SIZE_32);
        dma_channel_configure(led->chan, &led->chan_config, &led->pio->txf[sm], NULL, led->buff_size, false);
    }

    uint8_t offset = pio_add_program(led->pio, &led_pixel_program);
    led_pixel_program_init(led->pio, led->sm, offset, led->gpio_left_DI, LED_FREQ);

    return led;
}

void led_pixel_free(led_pixel_t* led) {
    free(led->buff);
    free(led);
}

static inline uint32_t interleave_16bits(uint8_t a, uint8_t b) {
    uint32_t x = 0;
    for(int i=7; i>=0; i--) {
        x = x<<1 | (a & 1<<i)>>i;
        x = x<<1 | (b & 1<<i)>>i;
    }
    return x;
}

static inline void encode_pixels(uint32_t* buff, uint32_t* left, uint32_t* right, uint8_t count, uint8_t buff_size) {
    uint32_t c, ci = 0;
    for(int i=0; i<count; i++) {
        uint32_t cl = left[i], cr = right[i];
        uint8_t clr = (cl & 0xff0000) >> 16,
            clg = (cl & 0xff00) >> 8,
            clb = (cl & 0xff),
            crr = (cr & 0xff0000) >> 16,
            crg = (cr & 0xff00) >> 8,
            crb = (cr & 0xff);
        c = interleave_16bits(crg, clg); // pack green
        buff[ci/2] = ci%2 ? buff[ci/2]|c : c<<16;
        ci++;
        c = interleave_16bits(crr, clr); // pack red
        buff[ci/2] = ci%2 ? buff[ci/2]|c : c<<16;
        ci++;
        c = interleave_16bits(crb, clb); // pack blue
        buff[ci/2] = ci%2 ? buff[ci/2]|c : c<<16;
        ci++;
    }
    for(; ci<2*buff_size; ci++) {
        buff[ci/2] = ci%2 ? buff[ci/2] : 0;
    }

}

void led_pixel_set(led_pixel_t* led, uint32_t* colors_left_rgb, uint32_t* colors_right_rgb) {
    encode_pixels(led->buff, colors_left_rgb, colors_right_rgb, led->count, led->buff_size);
    dma_channel_wait_for_finish_blocking(led->chan); // can be removed, since we plan to run it at large intervals
    pio_sm_set_pins(led->pio, led->sm, 0); // reset
    sleep_us(100);
    dma_channel_set_read_addr(led->chan, led->buff, true);
    led->on = true;
}

void led_pixel_set2(led_pixel_t* led, uint32_t* colors_left_rgb, uint32_t* colors_right_rgb) {
    encode_pixels(led->buff, colors_left_rgb, colors_right_rgb, led->count, led->buff_size);
    pio_sm_set_pins(led->pio, led->sm, 0); // reset
    sleep_us(100);
    for(int i=0; i<led->buff_size; i++) {
        pio_sm_put_blocking(led->pio, led->sm, led->buff[i]);
    }
    led->on = true;
}

void led_pixel_set_off(led_pixel_t* led) {
    if(!led->on) return;

    uint32_t cs[led->count];
    memset(cs, 0, 4*led->count);
    led_pixel_set(led, cs, cs);
    led->on = false;
}
