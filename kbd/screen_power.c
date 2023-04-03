#include <stdio.h>
#include <string.h>

#include "hw_model.h"
#include "screen_processor.h"

#define CONFIG_VERSION 0x01
#define FIELD_COUNT 2

static uint8_t backlight;
static uint8_t idle_minutes; // 0xFF means never
static flash_dataset_t* fd;

static uint8_t field;
static bool dirty;

static uint8_t select_next_value(uint8_t field) {
    switch(field) {
    case 0: // backlight
        return backlight = backlight>=90 ? 100 : backlight+10;
    case 1: // idle_minutes
        switch(idle_minutes) {
        case 5: return idle_minutes = 10;
        case 10: return idle_minutes = 30;
        case 30: return idle_minutes = 60;
        case 60: return idle_minutes = 120;
        case 120: return idle_minutes = 240;
        case 240: return idle_minutes = 0xFF;
        case 0xFF: return idle_minutes = 5;
        default: return 5;
        }
    default: return 0; // invalid
    }
}

static uint8_t select_prev_value(uint8_t field) {
    switch(field) {
    case 0: // backlight
        return backlight = backlight<=10 ? 0 : backlight-10;
    case 1: // idle_minutes
        switch(idle_minutes) {
        case 5: return idle_minutes = 0xFF;
        case 10: return idle_minutes = 5;
        case 30: return idle_minutes = 10;
        case 60: return idle_minutes = 30;
        case 120: return idle_minutes = 60;
        case 240: return idle_minutes = 120;
        case 0xFF: return idle_minutes = 240;
        default: return 5;
        }
    default: return 0; // invalid
    }
}

static uint8_t set_field(uint8_t field, uint8_t value) {
    switch(field) {
    case 0: // backlight
        return backlight = value;
    case 1: // idle_minutes
        return idle_minutes = value;
    default: return 0; // invalid
    }
}

static void save() {
    uint8_t* data = fd->data;
    data[0] = CONFIG_VERSION;
    data[1] = backlight;
    data[2] = idle_minutes;
    flash_store_save(fd);
}

static void draw_backlight(lcd_canvas_t* cv, uint16_t x, uint16_t y, bool selected) {
    char txt[8];
    sprintf(txt, "%3d %%", backlight);
    lcd_canvas_text(cv, x, y, txt, &lcd_font24, selected?RED:WHITE, LCD_BODY_BG);
}

static void draw_idle_minutes(lcd_canvas_t* cv, uint16_t x, uint16_t y, bool selected) {
    char txt[8];
    if(idle_minutes==0xFF) sprintf(txt, "Never");
    else if(idle_minutes>=60) sprintf(txt, "%d h", idle_minutes/60);
    else sprintf(txt, "%3d m", idle_minutes);
    lcd_canvas_text(cv, x, y, txt, &lcd_font24, selected?RED:WHITE, LCD_BODY_BG);
}

static void init_screen() {
    if(kbd_system.side != kbd_side_LEFT) return;

    lcd_canvas_t* cv = kbd_hw.lcd_body;
    lcd_canvas_clear(cv);

    char txt[16];
    sprintf(txt, "Power-%04d", fd->pos);
    lcd_canvas_text(cv, 65, 10, txt, &lcd_font16, BLUE, LCD_BODY_BG);

    lcd_canvas_text(cv, 20, 60, "Backlight", &lcd_font24, DARK_GRAY, LCD_BODY_BG);
    draw_backlight(cv, 135, 90, field==0);
    lcd_canvas_text(cv, 20, 130, "Idle Minutes", &lcd_font24, DARK_GRAY, LCD_BODY_BG);
    draw_idle_minutes(cv, 135, 160, field==1);
    lcd_display_body();
}

static void update_screen(uint8_t field, uint8_t sel_field) {
    if(kbd_system.side != kbd_side_LEFT) return;

    lcd_canvas_t* cv = lcd_new_shared_canvas(kbd_hw.lcd_body->buf, 85, 24, LCD_BODY_BG);

    if(field!=sel_field || sel_field==0) {
        draw_backlight(cv, 0, 0, sel_field==0);
        lcd_display_body_canvas(135, 90, cv);
        lcd_canvas_clear(cv);
    }

    if(field!=sel_field || sel_field==1) {
        draw_idle_minutes(cv, 0, 0, sel_field==1);
        lcd_display_body_canvas(135, 160, cv);
    }
    lcd_free_canvas(cv);

    if(dirty) {
        cv = lcd_new_shared_canvas(kbd_hw.lcd_body->buf, 10, 10, LCD_BODY_BG);
        lcd_canvas_circle(cv, 5, 5, 5, RED, 1, true);
        lcd_display_body_canvas(220, 10, cv);
        lcd_free_canvas(cv);
    }
}

static void init_data() {
    backlight = kbd_system.backlight;
    idle_minutes = kbd_system.idle_minutes;
}

void execute_screen_power(kbd_event_t event) {
    uint8_t* lreq = kbd_system.left_task_request;
    uint8_t* rreq = kbd_system.right_task_request;

    if(is_nav_event(event)) return;

    switch(event) {
    case kbd_screen_event_INIT:
        init_data();
        mark_left_request(kbd_config_screen_power);
        lreq[2] = 0;
        lreq[3] = field = 0;
        lreq[4] = backlight;
        lreq[5] = idle_minutes;
        dirty = false;
        break;
    case kbd_screen_event_LEFT:
    case kbd_screen_event_UP:
        mark_left_request(kbd_config_screen_power);
        lreq[2] = 1;
        lreq[3] = field;
        lreq[4] = field = field>0 ? field-1 : FIELD_COUNT-1;
        break;
    case kbd_screen_event_RIGHT:
    case kbd_screen_event_DOWN:
        mark_left_request(kbd_config_screen_power);
        lreq[2] = 1;
        lreq[3] = field;
        lreq[4] = field = field==FIELD_COUNT-1 ? 0 : field+1;
        break;
    case kbd_screen_event_SEL_PREV:
        mark_left_request(kbd_config_screen_power);
        lreq[2] = 2;
        lreq[3] = field;
        lreq[4] = select_prev_value(field);
        lreq[5] = dirty = true;
        break;
    case kbd_screen_event_SEL_NEXT:
        mark_left_request(kbd_config_screen_power);
        lreq[2] = 2;
        lreq[3] = field;
        lreq[4] = select_next_value(field);
        lreq[5] = dirty = true;
        break;
    case kbd_screen_event_SAVE:
        mark_left_request(kbd_config_screen_power);
        mark_right_request(kbd_config_screen_power);
        rreq[2] = lreq[2] = 3;
        rreq[3] = lreq[3] = field = 0;
        rreq[4] = lreq[4] = backlight;
        rreq[5] = lreq[5] = idle_minutes;
        dirty = false;
        break;
    default: break;
    }
}

void respond_screen_power(void) {
    uint8_t* req = kbd_system.side == kbd_side_LEFT ?
        kbd_system.left_task_request : kbd_system.right_task_request;
    switch(req[2]) {
    case 0: // init
    case 3: // save
        field = req[3];
        backlight = req[4];
        idle_minutes = req[5];
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

void init_config_screen_default_power() {
    init_data();

    fd = kbd_system.flash_datasets[get_screen_index(kbd_config_screen_power)];
    uint8_t* data = fd->data;
    memset(data, 0xFF, FLASH_DATASET_SIZE); // use 0xFF = erased state in flash
    data[0] = CONFIG_VERSION;
    data[1] = backlight;
    data[2] = idle_minutes;
}

void apply_config_screen_power() {
    uint8_t* data = fd->data;
    if(data[0]!=CONFIG_VERSION) return;
    backlight = kbd_system.backlight = data[1];
    idle_minutes = kbd_system.idle_minutes = data[2];
}
