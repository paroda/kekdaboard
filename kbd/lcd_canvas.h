#ifndef __LCD_CANVAS_H
#define __LCD_CANVAS_H

#include <stdbool.h>
#include "lcd_fonts.h"

/*
 * 0 0000  8 1000
 * 1 0001  9 1001
 * 2 0010  A 1010
 * 3 0011  B 1011
 * 4 0100  C 1100
 * 5 0101  D 1101
 * 6 0110  E 1110
 * 7 0111  F 1111
 */

// 16bit RGB colors (5-6-5)
#define WHITE          0xFFFF
#define BLACK          0x0000
#define RED            0xF800
#define GREEN          0x07E0
#define BLUE           0x001F
#define MAGENTA        0xF81F // red blue
#define YELLOW         0xFFE0 // red green
#define CYAN           0x07FF // green blue
#define BROWN          0xBC40
#define GRAY           0x8430
#define DARK_GRAY      0x4108

typedef struct {
    uint16_t* buf;
    uint16_t width;
    uint16_t height;
    uint16_t color;
    bool shared;
} lcd_canvas_t;

lcd_canvas_t* lcd_new_canvas(uint16_t width, uint16_t height, uint16_t color);

lcd_canvas_t* lcd_new_shared_canvas(uint16_t* buf, uint16_t width, uint16_t height, uint16_t color);

void lcd_free_canvas(lcd_canvas_t* canvas);

void lcd_canvas_clear(lcd_canvas_t* canvas);

void lcd_canvas_set_pixel(lcd_canvas_t* canvas, uint16_t x, uint16_t y, uint16_t color);

void lcd_canvas_point(lcd_canvas_t* canvas, uint16_t x, uint16_t y,
                      uint16_t color, uint8_t thickness);

void lcd_canvas_line(lcd_canvas_t* canvas, uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye,
                     uint16_t color, uint8_t thickness, bool dotted);

void lcd_canvas_rect(lcd_canvas_t* canvas, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                     uint16_t color, uint8_t thickness, bool fill);

void lcd_canvas_circle(lcd_canvas_t* canvas, uint16_t cx, uint16_t cy, uint16_t r,
                       uint16_t color, uint8_t thickness, bool fill);

void lcd_canvas_text(lcd_canvas_t* canvas, uint16_t x, uint16_t y, const char* text,
                     lcd_font_t* font, uint16_t color, uint16_t background);

#endif
