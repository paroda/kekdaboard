#ifndef __PIXEL_ANIM_H
#define __PIXEL_ANIM_H

#include <stdint.h>

typedef enum {
    pixel_anim_style_FIXED = 0,
    pixel_anim_style_FADE,
    pixel_anim_style_KEY_PRESS,
    pixel_anim_style_ROW_WAVE,
    pixel_anim_style_COUNT
} pixel_anim_style_t;

void pixel_anim_update(uint32_t* colors, uint8_t count,
                       uint32_t color, pixel_anim_style_t pixel_anim_style, uint8_t cycles);

void pixel_anim_reset();

#endif
