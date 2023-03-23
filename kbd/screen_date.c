#include <stdio.h>
#include <string.h>

#include "hw_model.h"
#include "screen_processor.h"

#define FIELD_COUNT 7

static rtc_datetime_t date;
static uint8_t field;
static bool dirty;

static uint16_t get_field(uint8_t field) {
    switch(field) {
    case 0: return date.year-2000;
    case 1: return date.month;
    case 2: return date.date;
    case 3: return date.weekday;
    case 4: return date.hour;
    case 5: return date.minute;
    case 6: return date.second;
    default: return 0; // invalid field
    }
}

static uint16_t set_field(uint8_t field, uint8_t value) {
    switch(field) {
    case 0: return (date.year = 2000+value)-2000;
    case 1: return date.month = value > 12 ? 1 : value < 1 ? 12 : value;
    case 2: return date.date = value > 31 ? 1 : value < 1 ? 31 : value;
    case 3: return date.weekday = value > 7 ? 1 : value < 1 ? 7 : value;
    case 4: return date.hour = value > 24 ? 23 : value > 23 ? 0 : value;
    case 5: return date.minute = value > 60 ? 59 : value > 59 ? 0 : value;
    case 6: return date.second = value > 60 ? 59 : value > 59 ? 0 : value;
    default: return value; // invalid field
    }
}

static void save() {
    rtc_set_time(kbd_hw.rtc, &date);
}

static void draw_field(lcd_canvas_t* cv, uint16_t x, uint16_t y, uint8_t field, bool selected) {
    char txt[8] = "YMDWHms";
    switch(field) {
    case 0: // year
        sprintf(txt, "%c: %4d", txt[field], 2000+get_field(field));
        break;
    case 3: // weekday
        sprintf(txt, "%c:  %s", txt[field], rtc_week[get_field(field)]);
        break;
    default:
        sprintf(txt, "%c:   %02d", txt[field], get_field(field));
        break;
    }
    lcd_canvas_text(cv, x, y, txt, &lcd_font24, selected?RED:WHITE, LCD_BODY_BG);
}

static void init_screen() {
    if(kbd_system.side != kbd_side_LEFT) return;

    lcd_canvas_t* cv = kbd_hw.lcd_body;
    lcd_canvas_clear(cv);
    for(uint8_t i=0; i<7; i++) {
        draw_field(cv, 60, 10+i*26, i, i==field);
    }
    lcd_display_body();
}

static void update_screen(uint8_t field, uint8_t sel_field) {
    if(kbd_system.side != kbd_side_LEFT) return;

    lcd_canvas_t* cv = lcd_new_shared_canvas(kbd_hw.lcd_body->buf, 120, 24, LCD_BODY_BG);

    if(field!=sel_field) {
        draw_field(cv, 0, 0, field, false);
        lcd_display_body_canvas(60, 10+field*26, cv);
        lcd_canvas_clear(cv);
    }
    draw_field(cv, 0, 0, sel_field, true);
    lcd_display_body_canvas(60, 10+sel_field*26, cv);
    lcd_free_canvas(cv);

    if(dirty) {
        cv = lcd_new_shared_canvas(kbd_hw.lcd_body->buf, 10, 10, LCD_BODY_BG);
        lcd_canvas_circle(cv, 5, 5, 5, RED, 1, true);
        lcd_display_body_canvas(220, 10, cv);
        lcd_free_canvas(cv);
    }
}

void execute_screen_date(kbd_event_t event) {
    uint8_t* lreq = kbd_system.left_task_request;
    uint8_t* rreq = kbd_system.right_task_request;

    if(is_nav_event(event)) return;

    switch(event) {
    case kbd_screen_event_INIT:
        date = kbd_system.date;
        mark_left_request(kbd_config_screen_date);
        lreq[2] = 0;
        lreq[3] = field = 0;
        memcpy(lreq+4, &date, sizeof(rtc_datetime_t));
        dirty = false;
        break;
    case kbd_screen_event_LEFT:
    case kbd_screen_event_UP:
        mark_left_request(kbd_config_screen_date);
        lreq[2] = 1;
        lreq[3] = field;
        lreq[4] = field = field>0 ? field-1 : FIELD_COUNT-1;
        break;
    case kbd_screen_event_RIGHT:
    case kbd_screen_event_DOWN:
        mark_left_request(kbd_config_screen_date);
        lreq[2] = 1;
        lreq[3] = field;
        lreq[4] = field = field==FIELD_COUNT-1 ? 0 : field+1;
        break;
    case kbd_screen_event_SEL_PREV:
        mark_left_request(kbd_config_screen_date);
        lreq[2] = 2;
        lreq[3] = field;
        lreq[4] = set_field(field, get_field(field) - 1);
        lreq[5] = dirty = true;
        break;
    case kbd_screen_event_SEL_NEXT:
        mark_left_request(kbd_config_screen_date);
        lreq[2] = 2;
        lreq[3] = field;
        lreq[4] = set_field(field, get_field(field) + 1);
        lreq[5] = dirty = true;
        break;
    case kbd_screen_event_SAVE:
        mark_left_request(kbd_config_screen_date);
        mark_right_request(kbd_config_screen_date);
        rreq[2] = lreq[2] = 3;
        rreq[3] = lreq[3] = field = 0;
        memcpy(lreq+4, &date, sizeof(rtc_datetime_t));
        memcpy(rreq+4, &date, sizeof(rtc_datetime_t));
        dirty = false;
        break;
    default: break;
    }
}

void respond_screen_date(void) {
    uint8_t* req = kbd_system.side == kbd_side_LEFT ?
        kbd_system.left_task_request : kbd_system.right_task_request;
    switch(req[2]) {
    case 0: // init
    case 3: // save
        field = req[3];
        memcpy(&date, req+4, sizeof(rtc_datetime_t));
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

void init_config_screen_default_date() {
    // No action
}

void apply_config_screen_date() {
    // No action
}
