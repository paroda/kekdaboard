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

static void anim_fade(uint32_t* colors_left, uint32_t* colors_right, uint8_t count, uint32_t color) {
    static uint8_t r = 0, g = 0, b = 0, d = 1;
    uint8_t R = (color & 0xff0000) >> 16;
    uint8_t G = (color & 0x00ff00) >> 8;
    uint8_t B = (color & 0x0000ff);
    if(d==1) {
        r = r<R ? r+1 : R;
        g = g<G ? g+1 : G;
        b = b<B ? b+1 : B;
        if(r==R && g==G && b==B) d=-1;
    } else {
        r = r>0 ? r-1 : 0;
        g = g>0 ? g-1 : 0;
        b = b>0 ? b-1 : 0;
        if(r==0 && g==0 && b==0) d=1;
    }
    color = r;
    color = (color<<8) | g;
    color = (color<<8) | b;
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
    case pixel_anim_style_FADE:
        anim_fade(colors_left, colors_right, count, color);
    default:
        break;
    }
}
