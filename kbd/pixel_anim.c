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
    static uint8_t r = 0, g = 0, b = 0, d = 1, R = 0, G = 0, B = 0, dr = 0, dg = 0, db = 0, cyc = 0, cr = 0, cg = 0, cb = 0;
    static uint32_t col = 0, i = 1;
    uint8_t x;
    cycles = cycles>0 ? cycles : 30; // can't be zero, default to 30 cycles, around 1 second
    if(color!=col || cycles!=cyc) {
        col = color;
        cyc = cycles;
        i = 1;
        r = g = b = 0;
        d = 1;
        R = (color & 0xff0000) >> 16;
        G = (color & 0x00ff00) >> 8;
        B = (color & 0x0000ff);
        dr = R/cycles;
        dg = G/cycles;
        db = B/cycles;
        x = R%cycles;
        cr = x>0 ? cycles/x : 0;
        x = G%cycles;
        cg = x>0 ? cycles/x : 0;
        x = B%cycles;
        cb = x>0 ? cycles/x : 0;
    }
    uint8_t dri = (i%cr)==0 ? dr+1 : dr;
    uint8_t dgi = (i%cg)==0 ? dg+1 : dg;
    uint8_t dbi = (i%cb)==0 ? db+1 : db;
    if(d==1) {
        r = r<(R-dri) ? r+dri : R;
        g = g<(G-dgi) ? g+dgi : G;
        b = b<(B-dbi) ? b+dbi : B;
        if(r==R && g==G && b==B) d=-1;
    } else {
        r = r>dri+dri ? r-dri : dri;
        g = g>dgi+dgi ? g-dgi : dgi;
        b = b>dgi+dgi ? b-dbi : dbi;
        if(r<=dri && g<=dgi && b<=dbi) d=1;
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
        anim_fade(colors_left, colors_right, count, color, cycles);
        break;
    default:
        anim_fixed(colors_left, colors_right, count, color);
        break;
    }
}
