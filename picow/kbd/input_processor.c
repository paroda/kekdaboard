#include <string.h>

#include "class/hid/hid.h"

#include "data_model.h"
#include "input_processor.h"

#define KEY_PRESS_MAX 16

static uint8_t key_layout_read(const uint8_t* key_press[KEY_PRESS_MAX]) {
    int row,col,k=0;
    uint8_t v;

    for(row=0;
        k<KEY_PRESS_MAX && row<hw_row_count;
        row++) {
        for(col=hw_col_count-1, v=kbd_system.core0.left_key_press[row];
            v>0 && k<KEY_PRESS_MAX && col>=0;
            col--, v>>=1) {
            if(v & 1) key_press[k++] = key_layout[row][col];
        }

        for(col=hw_col_count-1, v=kbd_system.core0.right_key_press[row];
            v>0 && k<KEY_PRESS_MAX && col>=0;
            col--, v>>=1) {
            if(v & 1) key_press[k++] = key_layout[row][hw_col_count+col];
        }
    }
    return k;
}

#define TRACK_KEY_COUNT 10

// keycodes tracked with new_key_press, old_key_press, cur_key_press
uint8_t track_keys[TRACK_KEY_COUNT] = {
    KBD_KEY_SUN,
    KBD_KEY_BACKLIGHT,
    KBD_KEY_PIXELS,
    HID_KEY_ARROW_UP,
    HID_KEY_ARROW_DOWN,
    HID_KEY_ARROW_LEFT,
    HID_KEY_ARROW_RIGHT,
    HID_KEY_SPACE,
    HID_KEY_ENTER,
    HID_KEY_ESCAPE,
};

typedef struct {
    uint8_t sun;
    uint8_t backlight;
    uint8_t pixels;
    uint8_t up;
    uint8_t down;
    uint8_t left;
    uint8_t right;
    uint8_t space;
    uint8_t enter;
    uint8_t escape;
} track_key_press_t;

track_key_press_t new_key_press; // newly pressed
track_key_press_t old_key_press; // previousl pressed
track_key_press_t cur_key_press; // currently pressed

static void update_track_key_press() {
    static bool init = true;
    if(init) {
        memset(&old_key_press, 0, sizeof(track_key_press_t));
        init = false;
    }
    uint8_t* nkp = (uint8_t*) &new_key_press;
    uint8_t* okp = (uint8_t*) &old_key_press;
    uint8_t* ckp = (uint8_t*) &cur_key_press;
    for(uint8_t i=0; i<TRACK_KEY_COUNT; i++) {
        nkp[i] = ckp[i] && !okp[i] ? 1 : 0;
        okp[i] = ckp[i];
    }
}

static void parse_modifier(uint8_t modifier, hid_report_out_keyboard_t* outk) {
    switch(modifier) {
    case KEYBOARD_MODIFIER_LEFTCTRL:
        outk->leftCtrl=true;
        break;
    case KEYBOARD_MODIFIER_LEFTSHIFT:
        outk->leftShift=true;
        break;
    case KEYBOARD_MODIFIER_LEFTALT:
        outk->leftAlt=true;
        break;
    case KEYBOARD_MODIFIER_LEFTGUI:
        outk->leftGui=true;
        break;
    case KEYBOARD_MODIFIER_RIGHTCTRL:
        outk->rightCtrl=true;
        break;
    case KEYBOARD_MODIFIER_RIGHTSHIFT:
        outk->rightShift=true;
        break;
    case KEYBOARD_MODIFIER_RIGHTALT:
        outk->rightAlt=true;
        break;
    case KEYBOARD_MODIFIER_RIGHTGUI:
        outk->rightGui=true;
        break;
    default: // skip
        break;
    }
}

static void parse_code(uint8_t code, uint8_t base_code,
                       uint8_t* n_key_codes, hid_report_out_keyboard_t* outk,
                       hid_report_out_mouse_t* outm) {
    switch (base_code) {
    case KBD_KEY_SUN:
        cur_key_press.sun = 1;
        break;
    case KBD_KEY_LEFT_MOON:
    case KBD_KEY_RIGHT_MOON:
        // no action
        break;
    default:
        switch(code) {
        case KBD_KEY_BACKLIGHT:
            cur_key_press.backlight = 1;
            break;
        case KBD_KEY_PIXELS:
            cur_key_press.pixels = 1;
            break;
        case KBD_KEY_MOUSE_LEFT:
            outm->left=true;
            break;
        case KBD_KEY_MOUSE_RIGHT:
            outm->right=true;
            break;
        case KBD_KEY_MOUSE_MIDDLE:
            outm->middle=true;
            break;
        case KBD_KEY_MOUSE_BACKWARD:
            outm->backward=true;
            break;
        case KBD_KEY_MOUSE_FORWARD:
            outm->forward=true;
            break;
        default: // normal keys
            outk->key_codes[*n_key_codes]=code;
            *n_key_codes = *n_key_codes + 1;
            // note keys for screen event
            for(int j=0; j<TRACK_KEY_COUNT; j++) {
                if(track_keys[j]==code) {
                    ((uint8_t*)&cur_key_press)[j] = 1;
                    break;
                }
            }
        }
        break;
    }
}

static inline int16_t cap16_value(int32_t v) {
    // cap the value to 16 bits
    return (v > 0x7fff) ? 0x7fff : (-v > 0x7fff) ? -0x7fff : v;
}

static inline int16_t add_cap16_value(int16_t v1, int16_t v2) {
    return cap16_value((int32_t)v1 + (int32_t)v2);
}

static bool parse_tb_motion(bool moon, bool shift, hid_report_out_mouse_t* outm) {
    // only parse the motion, no need to reset it to zero
    // the reset is taken care of by core0 processor, where it is read
    // and checked each time before calling input processor
    kbd_tb_motion_t* tb_motion = &kbd_system.core0.tb_motion;
    if(tb_motion->has_motion) {
        kbd_tb_config_t* tb_config = &kbd_system.core0.tb_config;
        uint8_t scale = moon ? tb_config->scroll_scale : tb_config->delta_scale;
        uint8_t quad_weight = moon ? tb_config->scroll_quad_weight : tb_config->delta_quad_weight;
        int32_t x,y,q,x_abs,y_abs;
        uint16_t m = tb_config->cpi / scale;
        x = tb_motion->dx/scale;
        q = (x<0 ? -x*x : x*x) / m;
        x = (x + quad_weight * q) / (1 + quad_weight);
        y = tb_motion->dy/scale;
        q = (y<0 ? -y*y : y*y) / m;
        y = (y + quad_weight * q) / (1 + quad_weight);
        if(shift) {
            x_abs = x < 0 ? -x : x;
            y_abs = y < 0 ? -y : y;
            if(x_abs < y_abs) {
                x = 0;
            } else {
                y = 0;
            }
        }
        if(moon) {
            outm->scrollX = cap16_value(x);
            outm->scrollY = cap16_value(-y);
        } else {
            outm->deltaX = cap16_value(x);
            outm->deltaY = cap16_value(y);
        }
        return true;
    }
    return false;
}

static kbd_event_t parse_config_screen_event(bool moon) {
    static bool exit = false;
    if(new_key_press.sun) return moon ? kbd_screen_event_PREV : kbd_screen_event_NEXT;
    if(new_key_press.up) return kbd_screen_event_UP;
    if(new_key_press.down) return kbd_screen_event_DOWN;
    if(new_key_press.left) return kbd_screen_event_LEFT;
    if(new_key_press.right) return kbd_screen_event_RIGHT;
    if(new_key_press.space) return moon ? kbd_screen_event_SEL_PREV : kbd_screen_event_SEL_NEXT;
    if(new_key_press.enter) return kbd_screen_event_SAVE;
    // exit on key up, not just key down
    if(new_key_press.escape) exit = true;
    if(exit && !cur_key_press.escape) {
        exit = false;
        return kbd_screen_event_EXIT;
    }
    return kbd_event_NONE;
}

// update the hid_report_out and return the screen event if any
kbd_event_t execute_input_processor() {
    // identify the raw left/right scan matrix
    const uint8_t* key_press[KEY_PRESS_MAX];
    uint8_t n = key_layout_read(key_press);

    // check for sun or moon
    bool sun=false, moon=false, lmoon=false, rmoon=false;
    int i;
    for(i=0; i<n; i++) {
        switch(key_press[i][1]) {
        case KBD_KEY_SUN:
            sun=true;
            break;
        case KBD_KEY_LEFT_MOON:
            lmoon=true;
            break;
        case KBD_KEY_RIGHT_MOON:
            rmoon=true;
            break;
        }
        if(sun && lmoon && rmoon) break; // no need to look further
    }
    moon = lmoon || rmoon;

    // initialize cleared
    hid_report_out_keyboard_t outk;
    memset(&outk, 0, sizeof(hid_report_out_keyboard_t));

    hid_report_out_mouse_t outm;
    memset(&outm, 0, sizeof(hid_report_out_mouse_t));

    memset(&cur_key_press, 0, sizeof(track_key_press_t));

    // parse keypress
    uint8_t n_key_codes=0;
    for(i=0; i<n; i++) {
        // read modifiers
        parse_modifier(key_press[i][0], &outk);
        // read key_codes
        uint8_t base_code = key_press[i][1];
        uint8_t moon_code = key_press[i][2];
        uint8_t code = (moon && moon_code) ? moon_code : base_code;
        parse_code(code, base_code, &n_key_codes, &outk, &outm);
    }

    // parse tb motion
    bool has_motion = parse_tb_motion(moon, outk.leftShift || outk.rightShift, &outm);

    // decide presence of user input
    bool has_events = (n_key_codes>0
                       || outk.leftCtrl  || outk.leftShift  || outk.leftAlt  || outk.leftGui
                       || outk.rightCtrl || outk.rightShift || outk.rightAlt || outk.rightGui
                       || outm.left || outm.right || outm.middle || outm.backward || outm.forward
                       || has_motion);

    // note key press events for screen
    update_track_key_press();

    hid_report_out_t* hid_report_out = &kbd_system.core0.hid_report_out;

    bool config_mode = is_config_screen(kbd_system.screen);
    if(config_mode) {
        // no hid report in config mode
        memset(hid_report_out, 0, sizeof(hid_report_out_t));

        // config screen event
        return parse_config_screen_event(moon);
    }

    // set the hid report
    hid_report_out->has_events = has_events;
    hid_report_out->keyboard = outk;
    outm.deltaX = add_cap16_value(outm.deltaX, hid_report_out->mouse.deltaX);
    outm.deltaY = add_cap16_value(outm.deltaY, hid_report_out->mouse.deltaY);
    outm.scrollX = add_cap16_value(outm.scrollX, hid_report_out->mouse.scrollX);
    outm.scrollY = add_cap16_value(outm.scrollY, hid_report_out->mouse.scrollY);
    hid_report_out->mouse = outm;

    // screen change event
    if(new_key_press.sun) {
        if(lmoon) return kbd_screen_event_CONFIG;
        if(rmoon) return kbd_screen_event_PREV;
        return kbd_screen_event_NEXT;
    }

    // backlight change event
    if(new_key_press.backlight)
        return lmoon ? kbd_backlight_event_LOW : kbd_backlight_event_HIGH;

    if(new_key_press.pixels)
        return kbd_led_pixels_TOGGLE;

    return kbd_event_NONE;
}
