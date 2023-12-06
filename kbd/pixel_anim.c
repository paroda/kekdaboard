#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "pixel_anim.h"

static void anim_fixed(uint32_t* colors_left, uint32_t* colors_right, uint8_t count, uint32_t color) {
    for(int i=0; i<count; i++) {
        colors_left[i] = color;
        colors_right[i] = color;
    }
}

void pixel_anim_update(uint32_t* colors_left, uint32_t* colors_right, uint8_t count, uint32_t color, pixel_anim_style_t pixel_anim_style, uint8_t cycles){
    switch (pixel_anim_style) {
    case pixel_anim_style_FIXED:
        anim_fixed(colors_left, colors_right, count, color);
        break;
    default:
        break;
    }
}
