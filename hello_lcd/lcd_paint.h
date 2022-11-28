#ifndef __LCD_PAINT_H
#define __LCD_PAINT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "lcd_fonts.h"

#define WHITE          0xFFFF
#define BLACK          0x0000
#define BLUE           0x001F
#define BRED           0XF81F
#define GRED           0XFFE0
#define GBLUE          0X07FF
#define RED            0xF800
#define MAGENTA        0xF81F
#define GREEN          0x07E0
#define CYAN           0x7FFF
#define YELLOW         0xFFE0
#define BROWN          0XBC40
#define BRRED          0XFC07
#define GRAY           0X8430

    typedef struct {
        uint16_t* buff;
        uint16_t width;
        uint16_t height;
        uint16_t color;
    } lcd_canvas_t;

    lcd_canvas_t* lcd_new_canvas(uint16_t width, uint16_t height, uint16_t color);

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

#ifdef __cplusplus
}
#endif

#endif
