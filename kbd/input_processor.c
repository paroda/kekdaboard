#include <string.h>

#include "class/hid/hid.h"

#include "input_processor.h"

#define KEY_PRESS_MAX 16

static uint8_t key_layout_read(const uint8_t* key_press[KEY_PRESS_MAX]) {
    int row,col,k=0;
    uint8_t v;

    for(row=0
            ; k<KEY_PRESS_MAX && row<KEY_ROW_COUNT
            ; row++) {
        for(col=KEY_COL_COUNT-1, v=kbd_system.left_key_press[row]
                ; v>0 && k<KEY_PRESS_MAX && col>=0
                ; col--, v>>=1) {
            if(v & 1) key_press[k++] = key_layout[row][col];
        }

        for(col=KEY_COL_COUNT-1, v=kbd_system.right_key_press[row]
                ; v>0 && k<KEY_PRESS_MAX && col>=0
                ; col--, v>>=1) {
            if(v & 1) key_press[k++] = key_layout[row][KEY_COL_COUNT+col];
        }
    }
    return k;
}

// update the hid_report_out and return the screen event if any

#define TB_SCROLL_SCALE 32
#define TB_DELTA_SCALE 4

#define TRACK_KEY_COUNT 9

// keycodes tracked with new_key_press, old_key_press, cur_key_press
uint8_t track_keys[TRACK_KEY_COUNT] = {
    KBD_KEY_SUN,
    KBD_KEY_BACKLIGHT,
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

static int16_t add_motion(int16_t dv, int16_t v) {
    int16_t d = dv<0 ? -dv : dv;
    d = (d+2*d*d/0x7F)/3;
    dv = dv<0 ? -d : d;
    return v+dv;
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
    if(base_code==KBD_KEY_SUN) {
        cur_key_press.sun = 1;
    } else if(base_code==KBD_KEY_LEFT_MOON || base_code==KBD_KEY_RIGHT_MOON) {
        // no action
    } else {
        switch(code) {
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
    }
}

static void parse_tb_motion(bool moon, bool shift, hid_report_out_mouse_t* outm) {
    if(kbd_system.right_tb_motion.has_motion) {
        uint8_t scale = moon ? TB_SCROLL_SCALE : TB_DELTA_SCALE;
        int16_t x = kbd_system.right_tb_motion.dx/scale;
        int16_t y = kbd_system.right_tb_motion.dy/scale;
        if(shift) {
            int16_t x_abs = x < 0 ? -x : x;
            int16_t y_abs = y < 0 ? -y : y;
            if(x_abs < y_abs) {
                x = 0;
            } else {
                y = 0;
            }
        }
        if(moon) {
            outm->scrollX = x;
            outm->scrollY = -y;
        } else {
            outm->deltaX = x;
            outm->deltaY = y;
        }
    }
}

static kbd_event_t parse_config_screen_event(bool moon) {
    if(new_key_press.sun) return moon ? kbd_screen_event_PREV : kbd_screen_event_NEXT;
    if(new_key_press.up) return kbd_screen_event_UP;
    if(new_key_press.down) return kbd_screen_event_DOWN;
    if(new_key_press.left) return kbd_screen_event_LEFT;
    if(new_key_press.right) return kbd_screen_event_RIGHT;
    if(new_key_press.space) return moon ? kbd_screen_event_SEL_PREV : kbd_screen_event_SEL_NEXT;
    if(new_key_press.enter) return kbd_screen_event_SAVE;
    if(new_key_press.escape) return kbd_screen_event_EXIT;
    return kbd_event_NONE;
}

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
    parse_tb_motion(moon, outk.leftShift || outk.rightShift, &outm);

    // decide presence of user input
    bool has_events = (n_key_codes>0
                       || outk.leftCtrl  || outk.leftShift  || outk.leftAlt  || outk.leftGui
                       || outk.rightCtrl || outk.rightShift || outk.rightAlt || outk.rightGui
                       || outm.left || outm.right || outm.middle || outm.backward || outm.forward
                       || kbd_system.right_tb_motion.has_motion);

    // note key press events for screen
    update_track_key_press();

    bool config_mode = kbd_system.screen & KBD_CONFIG_SCREEN_MASK;
    if(config_mode) {
        // no hid report in config mode
        memset(&kbd_system.hid_report_out, 0, sizeof(hid_report_out_t));

        // config screen event
        return parse_config_screen_event(moon);
    }

    // set the hid report
    kbd_system.hid_report_out.has_events = has_events;
    kbd_system.hid_report_out.keyboard = outk;
    outm.deltaX = add_motion(outm.deltaX, kbd_system.hid_report_out.mouse.deltaX);
    outm.deltaY = add_motion(outm.deltaY, kbd_system.hid_report_out.mouse.deltaY);
    outm.scrollX = add_motion(outm.scrollX, kbd_system.hid_report_out.mouse.scrollX);
    outm.scrollY = add_motion(outm.scrollY, kbd_system.hid_report_out.mouse.scrollY);
    kbd_system.hid_report_out.mouse = outm;

    // screen change event
    if(new_key_press.sun) {
        if(lmoon) return kbd_screen_event_CONFIG;
        if(rmoon) return kbd_screen_event_PREV;
        return kbd_screen_event_NEXT;
    }

    // backlight change event
    if(new_key_press.backlight)
        return lmoon ? kbd_backlight_event_LOW : kbd_backlight_event_HIGH;

    return kbd_event_NONE;
}
