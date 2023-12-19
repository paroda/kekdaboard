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

static void anim_fade(uint32_t* colors_left, uint32_t* colors_right, uint8_t count, uint32_t color, uint8_t cycles) {
    static uint32_t c = 0;
    static uint8_t n = 0, d = 1;
    static uint8_t i, R, G, B;
    static float AR, BR, AG, BG, AB, BB, CR, CG, CB;
    cycles = cycles>0 ? cycles : 20; // can't be zero, default to 20 cycles, around 1 second
    if(color!=c || cycles!=n) {
        c = color;
        n = cycles;
        i = 0;
        R = (color & 0xff0000) >> 16;
        G = (color & 0x00ff00) >> 8;
        B = (color & 0x0000ff);
        // ci = A + B * i + C * i^2
        AR = 0.1 * R;
        AG = 0.1 * G;
        AB = 0.1 * B;
        BR = 0.6 * R / n;
        BG = 0.6 * G / n;
        BB = 0.6 * B / n;
        CR = 0.3 * R / n / n;
        CG = 0.3 * G / n / n;
        CB = 0.3 * B / n / n;
    }
    uint8_t r = (uint8_t) (AR + BR * i + CR * i * i);
    uint8_t g = (uint8_t) (AG + BG * i + CG * i * i);
    uint8_t b = (uint8_t) (AB + BB * i + CB * i * i);
    i = d==1 ? i+1 : i-1;
    if(i==n) d = 0;
    else if(i==0) d = 1;
    color = r;
    color = (color<<8) | g;
    color = (color<<8) | b;
    for(int j=0; j<count; j++) {
        colors_left[j] = color;
        colors_right[j] = color;
    }
}

void pixel_anim_update(uint32_t* colors_left, uint32_t* colors_right, uint8_t count, uint32_t color, pixel_anim_style_t pixel_anim_style, uint8_t cycles){
    switch (pixel_anim_style) {
    case pixel_anim_style_FIXED:
        anim_fixed(colors_left, colors_right, count, color);
        break;
    case pixel_anim_style_FADE:
        anim_fade(colors_left, colors_right, count, color, cycles);
        break;
    default:
        anim_fixed(colors_left, colors_right, count, color);
        break;
    }
}
