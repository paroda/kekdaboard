#include <stdio.h>
#include <string.h>

#include "hw_model.h"
#include "screen_processor.h"

#define CONFIG_VERSION 0x01
#define FIELD_COUNT 8

typedef struct {
    uint8_t version;
    uint8_t color_red;
    uint8_t color_green;
    uint8_t color_blue;
    uint8_t anim_style;
    uint8_t anim_cycles;
} pixel_config_t;

static pixel_config_t pixel_config;
static flash_dataset_t* fd;

static uint8_t field;
static bool dirty;

// use 0x7f as the max value for each color component
// while the key switch leds can support 8bits for each color component
// the lcd display only supports 5-6-5 bits respectively for R-G-B
// hence, the UI selection will jump by 0b0100

static uint8_t select_next_value(uint8_t field) {
    uint8_t v;
    switch(field) {
    case 0: // color_red_H
        v = pixel_config.color_red & 0xf0;
        v = v>=0x70 ? 0x00 : v+0x10;
        return pixel_config.color_red = v | (pixel_config.color_red & 0x0f);
    case 1: // color_red_L
        v = pixel_config.color_red & 0b1111;
        v = v>=0b1100 ? 0b0000 : v+0b0100;
        return pixel_config.color_red = (pixel_config.color_red & 0xf0) | v;
    case 2: // color_green_H
        v = pixel_config.color_green & 0xf0;
        v = v>=0x70 ? 0x00 : v+0x10;
        return pixel_config.color_green = v | (pixel_config.color_green & 0x0f);
    case 3: // color_green_L
        v = pixel_config.color_green & 0b1111;
        v = v>=0b1100 ? 0b0000 : v+0b0100;
        return pixel_config.color_green = (pixel_config.color_green & 0xf0) | v;
    case 4: // color_blue_H
        v = pixel_config.color_blue & 0xf0;
        v = v>=0x70 ? 0x00 : v+0x10;
        return pixel_config.color_blue = v | (pixel_config.color_blue & 0x0f);
    case 5: // color_blue_L
        v = pixel_config.color_blue & 0b1111;
        v = v>=0b1100 ? 0b0000 : v+0b0100;
        return pixel_config.color_blue = (pixel_config.color_blue & 0xf0) | v;
    case 6: // anim_style
        return pixel_config.anim_style = pixel_config.anim_style<(pixel_anim_style_COUNT-1) ?
            (pixel_config.anim_style+1) : 0;
        break;
    case 7: // anim_cycles
        return pixel_config.anim_cycles = pixel_config.anim_cycles+1;
        break;
    default: return 0; // invalid
    }
}

static uint8_t select_prev_value(uint8_t field) {
    uint8_t v;
    switch(field) {
    case 0: // color_red_H
        v = pixel_config.color_red & 0xf0;
        v = v==0x00 ? 0x70 : v-0x10;
        return pixel_config.color_red = v | (pixel_config.color_red & 0x0f);
    case 1: // color_red_L
        v = pixel_config.color_red & 0b1111;
        v = v==0b0000 ? 0b1100 : v-0b0100;
        return pixel_config.color_red = (pixel_config.color_red & 0xf0) | v;
    case 2: // color_green_H
        v = pixel_config.color_green & 0xf0;
        v = v==0x00 ? 0x70 : v-0x10;
        return pixel_config.color_green = v | (pixel_config.color_green & 0x0f);
    case 3: // color_green_L
        v = pixel_config.color_green & 0b1111;
        v = v==0b0000 ? 0b1100 : v-0b0100;
        return pixel_config.color_green = (pixel_config.color_green & 0xf0) | v;
    case 4: // color_blue_H
        v = pixel_config.color_blue & 0xf0;
        v = v==0x00 ? 0x70 : v-0x10;
        return pixel_config.color_blue = v | (pixel_config.color_blue & 0x0f);
    case 5: // color_blue_L
        v = pixel_config.color_blue & 0b1111;
        v = v==0b0000 ? 0b1100 : v-0b0100;
        return pixel_config.color_blue = (pixel_config.color_blue & 0xf0) | v;
    case 6: // anim_style
        return pixel_config.anim_style = pixel_config.anim_style>0 ?
            (pixel_config.anim_style-1) : (pixel_anim_style_COUNT-1);
        break;
    case 7: // anim_cycles
        return pixel_config.anim_cycles = pixel_config.anim_cycles-1;
        break;
    default: return 0; // invalid
    }
}

static uint8_t set_field(uint8_t field, uint8_t value) {
    switch(field) {
    case 0: // color_red_H
    case 1: // color_red_L
        return pixel_config.color_red = value;
    case 2: // color_green_H
    case 3: // color_green_L
        return pixel_config.color_green = value;
    case 4: // color_blue_H
    case 5: // color_blue_H
        return pixel_config.color_blue = value;
    case 6: // anim_style
        return pixel_config.anim_style = value;
    case 7: // anim_cycles
        return pixel_config.anim_cycles = value;
    default: return 0; // invalid
    }
}

static void save() {
    memcpy(fd->data, &pixel_config, sizeof(pixel_config_t));
    flash_store_save(fd);
}

// TODO: update_screen init_screen

static void draw_color_component(lcd_canvas_t* cv, uint8_t component, uint16_t x, uint16_t y, uint8_t selected) {
    char* hex[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"};
    lcd_canvas_text(cv, x, y, hex[component>>4], &lcd_font24, selected==1?RED:WHITE, LCD_BODY_BG);
    lcd_canvas_text(cv, x+20, y, hex[component&0x0f], &lcd_font24, selected==2?RED:WHITE, LCD_BODY_BG);
}

static void draw_color_box(lcd_canvas_t* cv, uint8_t red, uint8_t green, uint8_t blue, uint16_t x, uint16_t y) {
    // the first bit is always 0, as limited to 0x7f
    // and the last two bits are also 0, as we jump selection by 0b0100
    // so we have 5 bits only for red, green or blue
    uint16_t r = red >> 2;
    uint16_t g = green >> 2;
    uint16_t b = blue >> 2;
    uint16_t color = (r << 11) | (g << 5) | b;
    lcd_canvas_rect(cv, x, y, 70, 24, BLACK, 0, true);
    lcd_canvas_rect(cv, x+2, y+2, 64, 20, color, 0, true);
}

static void draw_color(lcd_canvas_t* cv, uint16_t x, uint16_t y, uint8_t selected) {
    draw_color_component(cv, pixel_config.color_red, x, y, selected==0?1:(selected==1?2:0));
    draw_color_component(cv, pixel_config.color_green, x+50, y, selected==2?1:(selected==3?2:0));
    draw_color_component(cv, pixel_config.color_blue, x+100, y, selected==4?1:(selected==5?2:0));
    draw_color_box(cv, pixel_config.color_red, pixel_config.color_green, pixel_config.color_blue, x+150, y);
}

static void draw_anim_style(lcd_canvas_t* cv, uint16_t x, uint16_t y, bool selected) {
    lcd_canvas_text(cv, x, y, "Anim", &lcd_font24, DARK_GRAY, LCD_BODY_BG);

    char* anim;
    switch(pixel_config.anim_style) {
    case pixel_anim_style_FIXED:
        anim = "Fixed   ";
        break;
    case pixel_anim_style_FADE:
        anim = "Fade    ";
        break;
    case pixel_anim_style_KEY_PRESS:
        anim = "KeyPress";
        break;
    default:
        anim = "Error   ";
        break;
    }
    lcd_canvas_text(cv, x+80, y, anim, &lcd_font24, selected?RED:WHITE, LCD_BODY_BG);
}

static void draw_anim_cycles(lcd_canvas_t* cv, uint16_t x, uint16_t y, bool selected) {
    lcd_canvas_text(cv, x, y, "Cycles", &lcd_font24, DARK_GRAY, LCD_BODY_BG);

    char txt[16];
    sprintf(txt, "%3d", pixel_config.anim_cycles);
    lcd_canvas_text(cv, x+120, y, txt, &lcd_font24, selected?RED:WHITE, LCD_BODY_BG);
}

static void init_screen() {
    if(kbd_system.side != kbd_side_LEFT) return;

    lcd_canvas_t* cv = kbd_hw.lcd_body;
    lcd_canvas_clear(cv);

    char txt[16];
    sprintf(txt, "Pixels-%04d", fd->pos);
    lcd_canvas_text(cv, 43, 10, txt, &lcd_font16, BLUE, LCD_BODY_BG);

    draw_color(cv, 10, 60, field);
    draw_anim_style(cv, 10, 100, field==6);
    draw_anim_cycles(cv, 10, 150, field==7);

    lcd_display_body();
}

static void update_dirty() {
    if(dirty) {
        lcd_canvas_t* cv = lcd_new_shared_canvas(kbd_hw.lcd_body->buf, 10, 10, LCD_BODY_BG);
        lcd_canvas_circle(cv, 5, 5, 5, RED, 1, true);
        lcd_display_body_canvas(220, 10, cv);
        lcd_free_canvas(cv);
    }
}

static void draw_field(lcd_canvas_t* cv, uint8_t field, bool selected) {
    switch(field) {
    case 0: // red_H
    case 1: // red_L
    case 2: // green_H
    case 3: // green_L
    case 4: // blue_H
    case 5: // blue_L
        draw_color(cv, 0, 0, selected?field:0xff);
        lcd_display_body_canvas(10, 60, cv);
        break;
    case 6: // anim style
        draw_anim_style(cv, 0, 0, selected);
        lcd_display_body_canvas(10, 100, cv);
        break;
    case 7: // anim cycles
        draw_anim_cycles(cv, 0, 0, selected);
        lcd_display_body_canvas(10, 150, cv);
        break;
    default: break; //invalid
    }
}

static void update_screen(uint8_t field, uint8_t sel_field) {
    if(kbd_system.side != kbd_side_LEFT) return;

    lcd_canvas_t* cv = lcd_new_shared_canvas(kbd_hw.lcd_body->buf, 220, 24, LCD_BODY_BG);

    // fields 0-5 are the color components which are drawn together
    if(field!=sel_field && !(field<6 && sel_field<6)) {
        draw_field(cv, field, false);
        lcd_canvas_clear(cv);
    }

    draw_field(cv, sel_field, true);

    lcd_free_canvas(cv);

    update_dirty();
}

static void init_data() {
    pixel_config.version = CONFIG_VERSION;
    pixel_config.color_red = (uint8_t) ((kbd_system.pixel_color & 0xff0000) >> 16);
    pixel_config.color_green = (uint8_t) ((kbd_system.pixel_color & 0x00ff00) >> 8);
    pixel_config.color_blue = (uint8_t) (kbd_system.pixel_color & 0x0000ff);
    pixel_config.anim_style = kbd_system.pixel_anim_style;
    pixel_config.anim_cycles = kbd_system.pixel_anim_cycles;
}

/*
 * req array (32 bytes):
 *   0,1 : request identifier (set by marker)
 *   2 : command: 0-init, 1-select field, 2-set field, 3-save
 *   3 : current field
 *   4-: config data, when init/save
 *   4 : field to select, when selecting field
 *   4 : new value of current field, when setting field
 *   5 : if to show dirty, when setting field
 *
 * res array (32 bytes):
 *   0,1 : response identifier (set by marker)
 *   rest: not used
 */

void execute_screen_pixel(kbd_event_t event) {
    uint8_t* lreq = kbd_system.left_task_request;
    uint8_t* rreq = kbd_system.right_task_request;

    if(is_nav_event(event)) return;

    switch(event) {
    case kbd_screen_event_INIT:
        init_data();
        mark_left_request(kbd_config_screen_pixel);
        lreq[2] = 0;
        lreq[3] = field = 0;
        memcpy(lreq+4, &pixel_config, sizeof(pixel_config_t));
        dirty = false;
        break;
    case kbd_screen_event_LEFT:
    case kbd_screen_event_RIGHT:
    case kbd_screen_event_UP:
    case kbd_screen_event_DOWN:
        mark_left_request(kbd_config_screen_pixel);
        lreq[2] = 1; // select field
        lreq[3] = field; // current field
        switch(event) { // target field
        case kbd_screen_event_LEFT:
            lreq[4] = field = field==0 ? FIELD_COUNT-1 : field-1;
            break;
        case kbd_screen_event_RIGHT:
            lreq[4] = field = field==FIELD_COUNT-1 ? 0 : field+1;
            break;
        case kbd_screen_event_UP:
            lreq[4] = field = field>5 ? field-1 : FIELD_COUNT-1;
            break;
        case kbd_screen_event_DOWN:
            lreq[4] = field = field>5 ? (field==FIELD_COUNT-1 ? 0 : field+1) : 6;
            break;
        default: lreq[4] = field = 0; break;
        }
        break;
    case kbd_screen_event_SEL_PREV:
    case kbd_screen_event_SEL_NEXT:
        mark_left_request(kbd_config_screen_pixel);
        lreq[2] = 2;
        lreq[3] = field;
        lreq[4] = event==kbd_screen_event_SEL_PREV ? select_prev_value(field) : select_next_value(field);
        lreq[5] = dirty = true;
        break;
    case kbd_screen_event_SAVE:
        mark_left_request(kbd_config_screen_pixel);
        mark_right_request(kbd_config_screen_pixel);
        rreq[2] = lreq[2] = 3;
        rreq[3] = lreq[3] = field = 0;
        memcpy(lreq+4, &pixel_config, sizeof(pixel_config_t));
        memcpy(rreq+4, &pixel_config, sizeof(pixel_config_t));
        dirty = false;
        break;
    default: break;
    }
}

void respond_screen_pixel() {
    uint8_t* req = kbd_system.side == kbd_side_LEFT ?
        kbd_system.left_task_request : kbd_system.right_task_request;
    switch(req[2]) {
    case 0: // init
    case 3: // save
        field = req[3];
        memcpy(&pixel_config, req+4, sizeof(pixel_config_t));
        if(req[2]==3) save();
        dirty = false;
        init_screen();
        break;
    case 1: // select field
        field = req[4];
        update_screen(req[3], req[4]);
        break;
    case 2: // set field
        field = req[3];
        set_field(req[3], req[4]);
        dirty = req[5];
        update_screen(req[3], req[3]);
        break;
    default: break;
    }

    if(kbd_system.side == kbd_side_LEFT)
        mark_left_response();
    else
        mark_right_response();
}

void init_config_screen_default_pixel() {
    init_data();

    fd = kbd_system.flash_datasets[get_screen_index(kbd_config_screen_pixel)];
    memset(fd->data, 0xFF, FLASH_DATASET_SIZE); // use 0xFF = erased state in flash
    memcpy(fd->data, &pixel_config, sizeof(pixel_config_t));
}

void apply_config_screen_pixel() {
    if(fd->data[0]!=CONFIG_VERSION) return;

    memcpy(&pixel_config, fd->data, sizeof(pixel_config_t));

    kbd_system.pixel_color = (((uint32_t) pixel_config.color_red) << 16)
        | (((uint32_t) pixel_config.color_green) << 8)
        | ((uint32_t) pixel_config.color_blue);
    kbd_system.pixel_anim_style = pixel_config.anim_style;
    kbd_system.pixel_anim_cycles = pixel_config.anim_cycles;
}
