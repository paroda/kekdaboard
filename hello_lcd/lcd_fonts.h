#ifndef __LCD_FONTS_H
#define __LCD_FONTS_H

#include <stdint.h>

typedef struct {
    const uint8_t* table;
    uint16_t width;
    uint16_t height;
    uint8_t size; // byte count per char
} lcd_font_t;

extern lcd_font_t lcd_font8;  // 1kB  : 5x8   1x8x128
extern lcd_font_t lcd_font16; // 4 kB : 11x16 2x16x128
extern lcd_font_t lcd_font24; // 9 kB : 17x24 3x24x128

#endif
