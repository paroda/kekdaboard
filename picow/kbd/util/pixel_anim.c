#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "pixel_anim.h"

static bool flag_reset = false;

void pixel_anim_reset() {
    flag_reset = true;
}


static uint8_t interpolate(uint8_t a, uint8_t b, uint8_t steps, uint8_t range) {
    if(a==b) {
        return a;
    } else if(a>b) {
        uint8_t x = a;
        a = b;
        b = x;
        steps = range - steps;
    }
    return a + ((uint32_t)b-a)*steps*steps/range/range; // quadratic interpolation
}

static void to_rgb(uint32_t c, uint8_t* r, uint8_t* g, uint8_t* b) {
    *r = (c & 0xff0000) >> 16;
    *g = (c & 0x00ff00) >> 8;
    *b = (c & 0x0000ff);
}

static void from_rgb(uint32_t* c, uint8_t r, uint8_t g, uint8_t b) {
    *c = r;
    *c = ((*c)<<8) | g;
    *c = ((*c)<<8) | b;
}


static void anim_fixed(uint32_t* colors, uint8_t count, uint32_t color) {
    for(int i=0; i<count; i++) {
        colors[i] = color;
    }
}

static void anim_fade(uint32_t* colors, uint8_t count, uint32_t color, uint8_t cycles) {
    static uint32_t c = 0;
    static uint8_t n = 0, d = 1;
    static uint8_t i, R, G, B;
    static float AR, BR, AG, BG, AB, BB, CR, CG, CB;
    cycles = cycles>0 ? cycles : 20; // can't be zero, default to 20 cycles, around 1 second
    if(flag_reset) {
        c = 0;
        n = 0;
        d = 1;
        flag_reset = false;
    }
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
        colors[j] = color;
    }
}


static void from_spectrum(uint32_t* c, uint32_t* spectrum, uint8_t bandcount, uint8_t bandwidth, uint32_t pos) {
    uint8_t i, j, k, r0, g0, b0, r1, g1, b1;
    i = pos / bandwidth;
    j = (i+1)<bandcount ? i+1 : 0;
    k = pos % bandwidth;
    to_rgb(spectrum[i], &r0, &g0, &b0);
    to_rgb(spectrum[j], &r1, &g1, &b1);
    r0 = interpolate(r0, r1, k, bandwidth);
    g0 = interpolate(g0, g1, k, bandwidth);
    b0 = interpolate(b0, b1, k, bandwidth);
    from_rgb(c, r0, g0, b0);
}

static void anim_row_wave(uint32_t* colors, uint8_t count, uint32_t color, uint8_t cycles) {
    (void)color;

    uint32_t c, spectrum[6] = {0x7f0000, 0x7f7f00, 0x007f00, 0x007f7f, 0x00007f, 0x7f007f};
    static uint32_t pos = 0;
    cycles = cycles>0 ? cycles : 20; // can't be zero, default to 20 cycles, around 1 second
    uint32_t max_pos = 6 * cycles;
    if(flag_reset) {
        pos = 0;
        flag_reset = false;
    }
    for(uint8_t i=0; i<count; i++) {
        // one row has 7 leds, which would be same color
        if(i % 7 == 0) { // start of row
            uint8_t row = i/7;
            uint32_t p = pos + 5 * row;
            from_spectrum(&c, spectrum, 6, cycles, p % max_pos);
        }
        colors[i] = c;
    }
    pos = pos < max_pos ? pos+1 : 0; // after 6 band transitions, reset
}


void pixel_anim_update(uint32_t* colors, uint8_t count,
                       uint32_t color, pixel_anim_style_t pixel_anim_style, uint8_t cycles) {
    switch (pixel_anim_style) {
    case pixel_anim_style_FIXED:
        anim_fixed(colors, count, color);
        break;
    case pixel_anim_style_FADE:
        anim_fade(colors, count, color, cycles);
        break;
    case pixel_anim_style_ROW_WAVE:
        anim_row_wave(colors, count, color, cycles);
        break;
    default:
        anim_fixed(colors, count, color);
        break;
    }
}
