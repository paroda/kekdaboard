#include <stdio.h>
#include <string.h>

#include "../hw_model.h"
#include "../data_model.h"


#ifdef KBD_NODE_AP

void handle_screen_event_date(kbd_event_t event) {
    kbd_system_core0_t* c = &kbd_system.core0;
    uint8_t* req = c->left_task_request;

    if(is_nav_event(event)) return;

    init_task_request(req, &c->left_task_request_ts, kbd_info_screen_date);

    switch(event) {
    case kbd_screen_event_INIT:
        req[2] = 1;
        break;
    case kbd_screen_event_SAVE:
        req[2] = 2;
        break;
    case kbd_screen_event_LEFT:
    case kbd_screen_event_UP:
        req[2] = 3;
        break;
    case kbd_screen_event_RIGHT:
    case kbd_screen_event_DOWN:
        req[2] = 4;
        break;
    case kbd_screen_event_SEL_PREV:
        req[2] = 5;
        break;
    case kbd_screen_event_SEL_NEXT:
        req[2] = 6;
        break;
    default: break;
    }
}

#endif


#ifdef KBD_NODE_LEFT

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
    kbd_system.core0.date = date;
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
    lcd_canvas_t* cv = kbd_hw.lcd_body;
    lcd_canvas_clear(cv);
    for(uint8_t i=0; i<7; i++) {
        draw_field(cv, 60, 10+i*26, i, i==field);
    }
    lcd_display_body();
}

static void update_screen(uint8_t old_field, uint8_t field) {
    lcd_canvas_t* cv = lcd_new_shared_canvas(kbd_hw.lcd_body->buf, 120, 24, LCD_BODY_BG);

    if(old_field!=field) {
        draw_field(cv, 0, 0, old_field, false);
        lcd_display_body_canvas(60, 10+old_field*26, cv);
        lcd_canvas_clear(cv);
    }
    draw_field(cv, 0, 0, field, true);
    lcd_display_body_canvas(60, 10+field*26, cv);
    lcd_free_canvas(cv);

    if(dirty) {
        cv = lcd_new_shared_canvas(kbd_hw.lcd_body->buf, 10, 10, LCD_BODY_BG);
        lcd_canvas_circle(cv, 5, 5, 5, RED, 1, true);
        lcd_display_body_canvas(220, 10, cv);
        lcd_free_canvas(cv);
    }
}

void work_screen_task_date() {
    kbd_system_core0_t* c = &kbd_system.core0;
    uint8_t* req = c->task_request;
    uint8_t old_field;

    switch(req[2]) {
    case 1: //init
    case 2: // save
        if(req[2]==1) date = c->date; else save();
        field = 0;
        dirty = false;
        init_screen();
        break;
    case 3: // left field
    case 4: // right field
        old_field = field;
        field = (req[2]==3)
            ? (field>0 ? field-1 : FIELD_COUNT-1)
            : (field==FIELD_COUNT-1 ? 0 : field+1);
        update_screen(old_field, field);
        break;
    case 5: // prev value
    case 6: // next value
        set_field(field, get_field(field) + (req[2]==5?-1:1));
        dirty = true;
        update_screen(field, field);
        break;
    default: break;
    }
}

#else

void work_screen_task_date() {} // no action

#endif

void init_config_screen_data_date() {} // no action

void apply_config_screen_data_date() {} // no action
