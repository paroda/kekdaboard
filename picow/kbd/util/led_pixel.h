#ifndef __LED_PIXEL_H
#define __LED_PIXEL_H

#include <stdint.h>
#include <stdbool.h>

#include <hardware/pio.h>
#include <hardware/dma.h>

typedef struct {
    pio_hw_t* pio;
    uint8_t sm;
    uint8_t gpio_left_DI; // right gpio is assumed next consecutive pin

    int chan;
    dma_channel_config chan_config;
    uint32_t* buff; // buffer to hold pixel colors for dma
    uint8_t buff_size;
    bool on;

    uint8_t count; // total pixel count
} led_pixel_t;

/*
 * typically pio = pio0, sm = 0
 */
led_pixel_t* led_pixel_create(pio_hw_t* pio, uint8_t sm, uint8_t gpio_left_DI, uint8_t count);

void led_pixel_free(led_pixel_t* led);

void led_pixel_set(led_pixel_t* led, uint32_t* colors_left_rgb, uint32_t* colors_right_rgb);

void led_pixel_set2(led_pixel_t* led, uint32_t* colors_left_rgb, uint32_t* colors_right_rgb);

void led_pixel_set_off(led_pixel_t* led);

void led_pixel_finish_op(led_pixel_t* led);

#endif
