#include <stdlib.h>
#include "lcd_paint.h"


#ifdef __cplusplus
extern "C"
{
#endif


    lcd_canvas_t* lcd_new_canvas(uint16_t width, uint16_t height, uint16_t color) {
        lcd_canvas_t* canvas = (lcd_canvas_t*) malloc(sizeof(lcd_canvas_t));
        canvas->buff = (uint16_t*) malloc(width * height * sizeof(uint16_t));
        canvas->width = width;
        canvas->height = height;
        canvas->color = color;
        lcd_canvas_clear(canvas);
        return canvas;
    }

    void lcd_free_canvas(lcd_canvas_t* canvas) {
        free(canvas->buff);
        free(canvas);
    }

    void lcd_canvas_clear(lcd_canvas_t* canvas) {
        uint16_t* buff = canvas->buff;
        for(uint16_t y=0; y < canvas->height; y++) {
            for(uint16_t x=0; x < canvas->width; x++) {
                *(buff++) = canvas->color;
            }
        }
    }

    void lcd_canvas_pixel(lcd_canvas_t* canvas, uint16_t x, uint16_t y, uint16_t color) {
        if(x < canvas->width && y < canvas->height) {
            canvas->buff[x+y*canvas->width] = color;
        }
    }

    void lcd_canvas_point(lcd_canvas_t* canvas, uint16_t x, uint16_t y,
                          uint16_t color, uint8_t thickness) {
        if(x > canvas->width || y > canvas->height) return;
        int t1 = thickness >> 2;
        int t2 = thickness - t1;
        for(int dx = -t1; dx < t2; dx++) {
            int x2 = x + dx;
            if(x2 < 0) continue;
            for(int dy = -t1; dy < t2; dy++) {
                int y2 = y + dy;
                if(y2 < 0) continue;
                lcd_canvas_pixel(canvas, x2, y2, color);
            }
        }
    }

    void lcd_canvas_line(lcd_canvas_t* canvas, uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye,
                         uint16_t color, uint8_t thickness, bool dotted) {
        if(xs>canvas->width || xe>canvas->width || ys>canvas->height || ye>canvas->height) return;
        uint16_t x = xs;
        uint16_t y = ys;
        int dx = (int)xe - (int)xs >= 0 ? xe - xs : xs - xe;
        int dy = (int)ye - (int)ys <= 0 ? ye - ys : ys - ye;
        int sx = xs < xe ? 1 : -1;
        int sy = ys < ye ? 1 : -1;
        int err = dx + dy;
        int dot = 0;
        while(true) {
            dot++;
            int e2 = 2 * err;
            if(dotted) {
                int i = dot % (5*thickness);
                if(i<3*thickness) lcd_canvas_point(canvas, x, y, color, thickness);
            }
            else lcd_canvas_point(canvas, x, y, color, thickness);
            if(e2 >= dy) {
                if(x==xe) break;
                err += dy;
                x += sx;
            }
            if(e2 <= dx) {
                if(y==ye) break;
                err += dx;
                y += sy;
            }
        }
    }

    void lcd_canvas_rect(lcd_canvas_t* canvas, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         uint16_t color, uint8_t thickness, bool fill) {

    }

    void lcd_canvas_circle(lcd_canvas_t* canvas, uint16_t cx, uint16_t cy, uint16_t r,
                           uint16_t color, uint8_t thickness, bool fill) {

    }

    static void lcd_canvas_char(lcd_canvas_t* canvas, uint16_t xs, uint16_t ys, const char c,
                                lcd_font_t* font, uint16_t color, uint16_t background) {
        if(xs > canvas->width || ys > canvas->height) return;

        uint32_t char_offset = (c - ' ') * font->size;
        const unsigned char *p = font->table + char_offset;

        for(int row=0; row < font->height; row++) {
            for(int col=0; col < font->width; col++) {
                if(*p & (0x80 >> (col % 8)))
                    lcd_canvas_pixel(canvas, xs+col, ys+row, color);
                else if(background != canvas->color)
                    lcd_canvas_pixel(canvas, xs+col, ys+row, background);
                if(col % 8 == 7) p++; // move to next byte after 8 pixels [bits 0-7]
            }
            if(font->width % 8 != 0) p++; // row doesn't start in the middle of a byte
        }
    }

    void lcd_canvas_text(lcd_canvas_t* canvas, uint16_t xs, uint16_t ys, const char* text,
                         lcd_font_t* font, uint16_t color, uint16_t background) {
        const char* p = text;
        int x = xs, y = ys;

        while(*p != 0) {
            if(x+font->width > canvas->width) {
                x = xs;
                y += font->height;
            }
            if(y+font->height > canvas->height) return;
            lcd_canvas_char(canvas, x, y, *p, font, color, background);
            p++;
            x += font->width;
        }
    }


#ifdef __cplusplus
}
#endif
