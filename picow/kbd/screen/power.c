#include <stdio.h>
#include <string.h>

#include "../hw_model.h"
#include "../data_model.h"

#define THIS_SCREEN kbd_config_screen_power
#define CONFIG_VERSION 0x01

#ifdef KBD_NODE_AP
static flash_dataset_t* fd;
#else
static uint8_t* fd_pos;
#endif

#ifdef KBD_NODE_AP

void handle_screen_event_power(kbd_event_t event) {
    kbd_system_core0_t* c = &kbd_system.core0;
    uint8_t* req = c->task_request;
    uint8_t* lreq = c->left_task_request;
    uint8_t* lres = c->left_task_response;

    if(is_nav_event(event)) return;

    switch(event) {
    case kbd_screen_event_INIT:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 1;
        lreq[3] = 2;
        lreq[4] = kbd_system.backlight;
        lreq[5] = c->idle_minutes;
        break;
    case kbd_screen_event_SAVE:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 2;
        break;
    case kbd_screen_event_LEFT:
    case kbd_screen_event_UP:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 3;
        break;
    case kbd_screen_event_RIGHT:
    case kbd_screen_event_DOWN:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 4;
        break;
    case kbd_screen_event_SEL_PREV:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 5;
        break;
    case kbd_screen_event_SEL_NEXT:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 6;
        break;
    case kbd_screen_event_RESPONSE:
        if(lres[0] && lres[1]==THIS_SCREEN && lres[2]==1) {
            init_task_request(req, &c->task_request_ts, THIS_SCREEN);
            req[2] = 1;
            req[3] = 2;
            req[4] = kbd_system.backlight = lres[4];
            req[5] = c->idle_minutes = lres[5];
        }
        break;
    default: break;
    }
}

void work_screen_task_power() {
    kbd_system_core0_t* c = &kbd_system.core0;
    uint8_t* req = c->task_request;

    uint8_t* data = fd->data;
    switch(req[2]) {
    case 1: // save flash
        data[0] = CONFIG_VERSION;
        data[1] = req[4]; // backlight
        data[2] = req[5]; // idle_minutes
        flash_store_save(fd);
        break;
    default: break;
    }
}

#endif

#ifdef KBD_NODE_LEFT

#define FIELD_COUNT 2

static uint8_t backlight;
static uint8_t idle_minutes; // 0xFF means never
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
    lcd_canvas_t* cv = kbd_hw.lcd_body;
    lcd_canvas_clear(cv);

    char txt[16];
    sprintf(txt, "Power-%04d", *fd_pos);
    lcd_canvas_text(cv, 65, 10, txt, &lcd_font16, BLUE, LCD_BODY_BG);

    lcd_canvas_text(cv, 20, 60, "Backlight", &lcd_font24, DARK_GRAY, LCD_BODY_BG);
    draw_backlight(cv, 135, 90, field==0);
    lcd_canvas_text(cv, 20, 130, "Idle Minutes", &lcd_font24, DARK_GRAY, LCD_BODY_BG);
    draw_idle_minutes(cv, 135, 160, field==1);
    lcd_display_body();
}

static void update_screen(uint8_t field, uint8_t sel_field) {
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

void work_screen_task_power() {
    kbd_system_core0_t* c = &kbd_system.core0;
    uint8_t* req = c->task_request;
    uint8_t* res = c->task_response;

    uint8_t old_field;
    switch(req[2]) {
    case 1: // init
    case 2: // save
        if(req[2]==1) {
            backlight = req[4];
            idle_minutes = req[5];
        } else {
            res[2] = 1;
            res[3] = 2;
            res[4] = backlight;
            res[5] = idle_minutes;
        }
        field = 0; dirty = false;
        init_screen();
        break;
    case 3: // prev field
    case 4: // next field
        old_field = field;
        field = (req[2]==2)
            ? (field>0 ? field-1 : FIELD_COUNT-1)
            : (field==FIELD_COUNT-1 ? 0 : field+1);
        update_screen(old_field, field);
        break;
    case 5: // prev value
    case 6: // next value
        set_field(field, req[2]==4 ? select_prev_value(field) : select_next_value(field));
        dirty = true;
        update_screen(field, field);
        break;
    default: break;
    }
}

#endif

#ifdef KBD_NODE_RIGHT
void work_screen_task_power() {} // no action
#endif

#ifdef KBD_NODE_AP

void init_config_screen_data_power() {
    uint8_t si = get_screen_index(THIS_SCREEN);
    fd = kbd_system.core0.flash_datasets[si];
    uint8_t* data = fd->data;

    memset(data, 0xFF, FLASH_DATASET_SIZE); // use 0xFF = erased state in flash
    data[0] = CONFIG_VERSION;
    data[1] = kbd_system.backlight;
    data[2] = kbd_system.core0.idle_minutes;
}

void apply_config_screen_data_power() {
    uint8_t* data = fd->data;
    if(data[0]!=CONFIG_VERSION) return;
    kbd_system.backlight = data[1];
    kbd_system.core0.idle_minutes = data[2];
}

#else // left/right

void init_config_screen_data_power() {
    uint8_t si = get_screen_index(THIS_SCREEN);
    fd_pos = kbd_system.core0.flash_data_pos+si;
}
void apply_config_screen_data_power() {} // no action

#endif
