#ifndef __LCD_ST7789_H
#define __LCD_ST7789_H

#include <stdint.h>
#include <stdbool.h>

#include "master_spi.h"
#include "lcd_canvas.h"

/*
 * The ST7789 driver has a resolution of 240(W)x320(H) pixels.
 * Howeve, the physical screen may be smaller, which should be specified
 * while initializing with lcd_init().
 */

#define LCD_RES_WIDTH 240
#define LCD_RES_HEIGHT 320

typedef enum {
    lcd_orient_Normal = 0u,
    lcd_orient_Left =   1u,   // rotate 90 left
    lcd_orient_Right =  2u    // rotate 90 right
} lcd_orient_t;

typedef struct {
    master_spi_t* m_spi;

    uint8_t spi_slave_id;

    uint8_t gpio_DC;
    uint8_t gpio_RST; // set 0xFF to use software reset
    uint8_t gpio_BL;

    uint8_t backlight_level; // 0-100%

    bool on_DC; // true DC=true data mode, false DC=false command mode

    uint8_t pwm_slice_num_BL;
    uint8_t pwm_chan_BL;

    uint16_t width;
    uint16_t height;
    lcd_orient_t orient;

    // if using a smaller size like 240x240 instead of 240x320
    // then need to offset the window properly for left rotation.
    uint16_t origin_x;
    uint16_t origin_y;
} lcd_t;

/*
 * Specify the actual size of the physical screen.
 * View can be rotated 90 deg left or right using orient parameter.
 * NOTE: Do not swap the width and height for rotated view. That is
 *       automatically taken care of internally.
 */
lcd_t* lcd_create(master_spi_t* m_spi,
                  uint8_t gpio_CS, uint8_t gpio_DC, uint8_t gpio_RST, uint8_t gpio_BL,
                  uint16_t width, uint16_t height, lcd_orient_t orient);

void lcd_free(lcd_t* lcd);

void lcd_orient(lcd_t* lcd, lcd_orient_t orient);

void lcd_clear(lcd_t* lcd, uint16_t color);

void lcd_display_canvas(lcd_t* lcd, uint16_t xs, uint16_t ys, lcd_canvas_t* canvas);

void lcd_set_backlight_level(lcd_t* lcd, uint8_t value); // value: 0-100%

void print_lcd(lcd_t* lcd);

#endif
