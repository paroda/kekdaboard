#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "led_pixel.pio.h"
#include "led_pixel.h"

#define LED_FREQ 800000 // 800kHz

led_pixel_t* led_pixel_create(pio_hw_t* pio, uint8_t sm, uint8_t gpio_DI, uint8_t count) {
    led_pixel_t* led = (led_pixel_t*) malloc(sizeof(led_pixel_t));
    led->pio = pio;
    led->sm = sm;
    led->gpio_DI = gpio_DI;
    led->count = count;
    led->on = true;
    // as many as `count` pixels, each 3 bytes of color (rgb)
    // one word (32 bit) holds 4 bytes of color
    // hence (3*N/4) as many words needed.
    // And add one more to handle round down odd number of pixels.
    uint32_t n = 3 * (uint32_t) led->count;
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
    led_pixel_program_init(led->pio, led->sm, offset, led->gpio_DI, LED_FREQ);

    return led;
}

void led_pixel_free(led_pixel_t* led) {
    free(led->buff);
    free(led);
}

static inline void encode_pixels(uint32_t* buff, uint32_t* rgb, uint8_t count, uint8_t buff_size) {
    uint8_t i,j;
    uint8_t* b = (uint8_t*)buff;
    memset(buff, 0, buff_size*4);
    for(i=0,j=0; i<count; i++) {
        b[j++] = (rgb[i]>>8) & 0xff;  // green
        b[j++] = (rgb[i]>>16) & 0xff; // red
        b[j++] = (rgb[i]) & 0xff;     // blue
    }
    for(i=0,j=0; i<buff_size; i++) {
        uint32_t c = b[j++];
        c = (c<<8) | b[j++];
        c = (c<<8) | b[j++];
        c = (c<<8) | b[j++];
        buff[i] = c;
    }
}

void led_pixel_set(led_pixel_t* led, uint32_t* colors_rgb) {
    encode_pixels(led->buff, colors_rgb, led->count, led->buff_size);
    dma_channel_set_read_addr(led->chan, led->buff, true);
    dma_channel_wait_for_finish_blocking(led->chan);
    pio_sm_set_pins(led->pio, led->sm, 0); // reset, 0 for > 50ms
    sleep_us(80);
    pio_sm_set_pins(led->pio, led->sm, 1); // reset, 0 for > 50ms
    sleep_us(1);
    pio_sm_set_pins(led->pio, led->sm, 0); // reset, 0 for > 50ms
    led->on = true;
}

void led_pixel_set2(led_pixel_t* led, uint32_t* colors_rgb) {
    encode_pixels(led->buff, colors_rgb, led->count, led->buff_size);
    for(int i=0; i<led->buff_size; i++) {
        pio_sm_put_blocking(led->pio, led->sm, led->buff[i]);
    }
    pio_sm_set_pins(led->pio, led->sm, 0); // reset, 0 for > 50ms
    sleep_us(80);
    pio_sm_set_pins(led->pio, led->sm, 1); // reset, 0 for > 50ms
    sleep_us(1);
    pio_sm_set_pins(led->pio, led->sm, 0); // reset, 0 for > 50ms
    led->on = true;
}

void led_pixel_set_off(led_pixel_t* led) {
    if(!led->on) return;

    uint32_t cs[led->count] = {};
    led_pixel_set(led, cs);
    led->on = false;
}
