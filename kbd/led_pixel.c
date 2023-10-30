#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "led_pixel.pio.h"
#include "led_pixel.h"

#define LED_FREQ 800000 // 800kHz

static inline void led_pixel_setup_dma(int* chan, dma_channel_config* chan_config, pio_hw_t* pio, uint8_t sm, uint8_t count) {
    
}

led_pixel_t* led_pixel_create(pio_hw_t* pio, uint8_t sm, uint8_t gpio_left_DI, uint8_t gpio_right_DI, uint8_t count) {
    led_pixel_t* led = (led_pixel_t*) malloc(sizeof(led_pixel_t));
    led->pio = pio;
    led->sm = sm;
    led->gpio_left_DI = gpio_left_DI;
    led->gpio_right_DI = gpio_right_DI;
    led->count = count;
    led->buff = (uint8_t*) malloc(2*led->count*3); // 2 sides, each with `count` pixels, each 3 bytes of color (rgb)

    led->chan = dma_claim_unused_channel(false);
    if(led->chan) 
    {
        led->chan_config = dma_channel_get_default_config(led->chan);
        channel_config_set_dreq(&led->chan_config, pio_get_dreq(led->pio, led->sm, true));
        channel_config_set_read_increment(&led->chan_config, true);
        // transfer 2 bytes at a time (one byte for each side)
        // each led pixel has 3 bytes of color => transfer 3 times the number of pixels
        channel_config_set_transfer_data_size(&led->chan_config, DMA_SIZE_16);
        dma_channel_configure(led->chan, &led->chan_config, &led->pio->txf[sm], NULL, 3*count, false);
    }
    
    uint8_t offset = pio_add_program(pio, &led_pixel_program);
    led_pixel_program_init(pio, sm, offset, gpio_left_DI, LED_FREQ);

    return led;
}

void led_pixel_free(led_pixel_t* led) {
    free(led->buff);
    free(led);
}

static inline uint32_t ugrb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
        ((uint32_t) (g) << 16) |
        ((uint32_t) (r) << 8) |
        (uint32_t) (b);
}

static inline uint16_t interleave_16bits(uint8_t a, uint8_t b) {
    uint16_t x = 0;
    for(int i=7; i>=0; i--) {
        x = x<<1 | (a & 1<<i)>>i;
        x = x<<1 | (b & 1<<i)>>i;
    }
    return x;
}

static inline void encode_pixels(uint8_t* buff, uint32_t* left, uint32_t* right, uint8_t count) {    
    for(int i=0; i<count; i++) {
        uint32_t cl = left[i], cr = right[i];
        uint8_t clr = (cl & 0xff0000) >> 16,
            clg = (cl & 0xff00) >> 8,
            clb = (cl & 0xff),
            crr = (cr & 0xff0000) >> 16,
            crg = (cr & 0xff00) >> 8,
            crb = (cr & 0xff);
        uint16_t* p = (uint16_t*) (buff+(6*i));
        p[0] = interleave_16bits(clg, crg); // pack green
        p[1] = interleave_16bits(clr, crr); // pack red
        p[2] = interleave_16bits(clb, crb); // pack blue
    }
}

void led_pixel_set(led_pixel_t* led, uint32_t* colors_left_rgb, uint32_t* colors_right_rgb) {
    encode_pixels(led->buff, colors_left_rgb, colors_right_rgb, led->count);
    dma_channel_wait_for_finish_blocking(led->chan); // can be removed, since we plan to run it at large intervals
    dma_channel_set_read_addr(led->chan, led->buff, true);
}
