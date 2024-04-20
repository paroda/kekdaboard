#include <stdio.h>
#include <string.h>

#include "../hw_model.h"
#include "../data_model.h"

#define THIS_SCREEN kbd_config_screen_pixel
#define CONFIG_VERSION 0x01

typedef struct {
    uint8_t version;
    uint8_t color_red;
    uint8_t color_green;
    uint8_t color_blue;
    uint8_t anim_style;
    uint8_t anim_cycles;
} pixel_config_t;

static pixel_config_t pixel_config;

#ifdef KBD_NODE_AP

static flash_dataset_t* fd;

void handle_screen_event_pixel(kbd_event_t event) {
    kbd_system_core1_t* c = &kbd_system.core1;
    uint8_t* lreq = c->left_task_request;
    uint8_t* rreq = c->right_task_request;
    uint8_t* lres = c->left_task_response;

    if(is_nav_event(event)) return;

    switch(event) {
    case kbd_screen_event_INIT:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        init_task_request(rreq, &c->right_task_request_ts, THIS_SCREEN);
        lreq[2] = rreq[2] = 1;
        lreq[3] = fd->pos;
        memcpy(lreq+4, &pixel_config, sizeof(pixel_config_t));
        break;
    case kbd_screen_event_SAVE:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 2;
        break;
    case kbd_screen_event_LEFT:
    case kbd_screen_event_RIGHT:
    case kbd_screen_event_UP:
    case kbd_screen_event_DOWN:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 3;
        lreq[3] = 1;
        lreq[4] = event;
        break;
    case kbd_screen_event_SEL_PREV:
    case kbd_screen_event_SEL_NEXT:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 4;
        lreq[3] = 1;
        lreq[4] = event;
        break;
    case kbd_screen_event_RESPONSE:
        if(lres[0] && lres[1]==THIS_SCREEN && lres[2]==1) {
            // save to flash
            memcpy(&pixel_config, lres+4, sizeof(pixel_config_t));
            memcpy(fd->data, &pixel_config, sizeof(pixel_config_t));
            flash_store_save(fd);
            // show on lcd
            init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
            lreq[2] = 1;
            lreq[3] = fd->pos;
            memcpy(lreq+4, fd->data, sizeof(pixel_config_t));
        }
        break;
    default: break;
    }
}

#endif

#ifdef KBD_NODE_LEFT

#define FIELD_COUNT 8

static uint8_t fd_pos;
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
    lcd_canvas_t* cv = kbd_hw.lcd_body;
    lcd_canvas_clear(cv);

    char txt[16];
    sprintf(txt, "Pixels-%04d", fd_pos);
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

void work_screen_task_pixel() {
    kbd_system_core1_t* c = &kbd_system.core1;
    uint8_t* req = c->task_request;
    uint8_t* res = c->task_response;

    uint8_t old_field;
    switch(req[2]) {
    case 1: // init
        fd_pos = req[3];
        memcpy(&pixel_config, req+4, sizeof(pixel_config_t));
        field = 0; dirty = false;
        init_screen();
        break;
    case 2: // save
        res[2] = 1;
        res[3] = fd_pos;
        memcpy(res+4, &pixel_config, sizeof(pixel_config_t));
        break;
    case 3: // select field
        old_field = field;
        switch(req[4]) {
        case kbd_screen_event_LEFT:
            field = field==0 ? FIELD_COUNT-1 : field-1;
            break;
        case kbd_screen_event_RIGHT:
            field = field==FIELD_COUNT-1 ? 0 : field+1;
            break;
        case kbd_screen_event_UP:
            field = field>5 ? field-1 : FIELD_COUNT-1;
            break;
        case kbd_screen_event_DOWN:
            field = field>5 ? (field==FIELD_COUNT-1 ? 0 : field+1) : 6;
            break;
        default: break;
        }
        update_screen(old_field, field);
        break;
    case 4: // select value
        set_field(field, req[4]==kbd_screen_event_SEL_PREV
                  ? select_prev_value(field)
                  : select_next_value(field));
        dirty = true;
        update_screen(field, field);
        break;
    default: break;
    }
}

#endif

#ifdef KBD_NODE_RIGHT

void work_screen_task_pixel() {} // no action

#endif

void init_config_screen_data_pixel() {
    uint8_t si = get_screen_index(THIS_SCREEN);
#ifdef KBD_NODE_AP
    fd = kbd_system.core1.flash_datasets[si];
    uint8_t* data = fd->data;
#else
    uint8_t* data = kbd_system.core1.flash_data[si];
#endif

    memset(data, 0xFF, KBD_TASK_DATA_SIZE); // use 0xFF = erased state in flash

    kbd_pixel_config_t* pc = &kbd_system.core1.pixel_config;
    pixel_config.version = CONFIG_VERSION;
    pixel_config.color_red = (uint8_t) ((pc->color & 0xff0000) >> 16);
    pixel_config.color_green = (uint8_t) ((pc->color & 0x00ff00) >> 8);
    pixel_config.color_blue = (uint8_t) (pc->color & 0x0000ff);
    pixel_config.anim_style = pc->anim_style;
    pixel_config.anim_cycles = pc->anim_cycles;
    memcpy(data, &pixel_config, sizeof(pixel_config_t));
}

void apply_config_screen_data_pixel() {
#ifdef KBD_NODE_AP
    uint8_t* data = fd->data;
#else
    uint8_t si = get_screen_index(THIS_SCREEN);
    uint8_t* data = kbd_system.core1.flash_data[si];
#endif
    if(data[0]!=CONFIG_VERSION) return;

    memcpy(&pixel_config, data, sizeof(pixel_config_t));

    kbd_pixel_config_t* pc = &kbd_system.core1.pixel_config;
    pc->color = (((uint32_t) pixel_config.color_red) << 16)
        | (((uint32_t) pixel_config.color_green) << 8)
        | ((uint32_t) pixel_config.color_blue);
    pc->anim_style = pixel_config.anim_style;
    pc->anim_cycles = pixel_config.anim_cycles;
}
