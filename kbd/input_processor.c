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

#define TB_SCROLL_SCALE 64
#define TB_DELTA_SCALE 8

#define TRACK_KEY_COUNT 9

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

void update_track_key_press() {
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

int16_t add_motion(int16_t v1, int16_t v2) {
    v1 = v1+v2;
    return v1<-127 ? -127 : v1>127 ? 127 : v1;
}

kbd_event_t execute_input_processor() {
    const uint8_t* key_press[KEY_PRESS_MAX];
    uint8_t n = key_layout_read(key_press);

    bool sun=false, moon=false, lmoon=false, rmoon=false;
    int i,j;
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

    hid_report_out_keyboard_t outk;
    memset(&outk, 0, sizeof(hid_report_out_keyboard_t));
    hid_report_out_mouse_t outm;
    memset(&outm, 0, sizeof(hid_report_out_mouse_t));

    memset(&cur_key_press, 0, sizeof(track_key_press_t));

    uint8_t n_key_codes=0;
    for(i=0; i<n; i++) {
        // read modifiers
        switch(key_press[i][0]) {
        case KEYBOARD_MODIFIER_LEFTCTRL:
            outk.leftCtrl=true;
            break;
        case KEYBOARD_MODIFIER_LEFTSHIFT:
            outk.leftShift=true;
            break;
        case KEYBOARD_MODIFIER_LEFTALT:
            outk.leftAlt=true;
            break;
        case KEYBOARD_MODIFIER_LEFTGUI:
            outk.leftGui=true;
            break;
        case KEYBOARD_MODIFIER_RIGHTCTRL:
            outk.rightCtrl=true;
            break;
        case KEYBOARD_MODIFIER_RIGHTSHIFT:
            outk.rightShift=true;
            break;
        case KEYBOARD_MODIFIER_RIGHTALT:
            outk.rightAlt=true;
            break;
        case KEYBOARD_MODIFIER_RIGHTGUI:
            outk.rightGui=true;
            break;
        default: // skip
            break;
        }
        // read key_codes
        uint8_t code = (moon && key_press[i][2]) ? key_press[i][2] : key_press[i][1];
        if(key_press[i][1]==KBD_KEY_SUN
           || key_press[i][1]==KBD_KEY_LEFT_MOON
           || key_press[i][1]==KBD_KEY_RIGHT_MOON) {
            if(sun) cur_key_press.sun = 1;
        } else {
            switch(code) {
            case KBD_KEY_MOUSE_LEFT:
                outm.left=true;
                break;
            case KBD_KEY_MOUSE_RIGHT:
                outm.right=true;
                break;
            case KBD_KEY_MOUSE_MIDDLE:
                outm.middle=true;
                break;
            case KBD_KEY_MOUSE_BACKWARD:
                outm.backward=true;
                break;
            case KBD_KEY_MOUSE_FORWARD:
                outm.forward=true;
                break;
            default: // normal keys
                outk.key_codes[n_key_codes++]=code;
                // note keys for screen event
                for(j=0; j<TRACK_KEY_COUNT; j++) {
                    if(track_keys[j]==code) {
                        ((uint8_t*)&cur_key_press)[j] = 1;
                        break;
                    }
                }
            }
        }
    }

    if(kbd_system.right_tb_motion.has_motion) {
        if(moon) {
            outm.scrollX = kbd_system.right_tb_motion.dx/TB_SCROLL_SCALE;
            outm.scrollY = kbd_system.right_tb_motion.dy/TB_SCROLL_SCALE;
        } else {
            outm.deltaX = kbd_system.right_tb_motion.dx/TB_DELTA_SCALE;
            outm.deltaY = kbd_system.right_tb_motion.dy/TB_DELTA_SCALE;
        }
    }

    bool has_events = (n_key_codes>0
                       || outk.leftCtrl  || outk.leftShift  || outk.leftAlt  || outk.leftGui
                       || outk.rightCtrl || outk.rightShift || outk.rightAlt || outk.rightGui
                       || outm.left || outm.right || outm.middle || outm.backward || outm.forward
                       || kbd_system.right_tb_motion.has_motion);

    // note key press events for screen
    update_track_key_press();

    kbd_event_t event = kbd_event_NONE;

    bool config_mode = kbd_system.screen & KBD_CONFIG_SCREEN_MASK;
    if(config_mode) {
        // no hid report in config mode
        memset(&kbd_system.hid_report_out, 0, sizeof(hid_report_out_t));

        // config screen event
        if(new_key_press.sun) {
            event = moon ? kbd_screen_event_PREV : kbd_screen_event_NEXT;
        } else if(new_key_press.up) {
            event = kbd_screen_event_UP;
        } else if(new_key_press.down) {
            event = kbd_screen_event_DOWN;
        } else if(new_key_press.left) {
            event = kbd_screen_event_LEFT;
        } else if(new_key_press.right) {
            event = kbd_screen_event_RIGHT;
        } else if(new_key_press.space) {
            event = moon ? kbd_screen_event_SEL_PREV : kbd_screen_event_SEL_NEXT;
        } else if(new_key_press.enter) {
            event = kbd_screen_event_SAVE;
        } else if(new_key_press.escape) {
            event = kbd_screen_event_EXIT;
        }
    } else {
        kbd_system.hid_report_out.has_events = has_events;
        kbd_system.hid_report_out.keyboard = outk;
        outm.deltaX = add_motion(outm.deltaX, kbd_system.hid_report_out.mouse.deltaX);
        outm.deltaY = add_motion(outm.deltaY, kbd_system.hid_report_out.mouse.deltaY);
        outm.scrollX = add_motion(outm.scrollX, kbd_system.hid_report_out.mouse.scrollX);
        outm.scrollY = add_motion(outm.scrollY, kbd_system.hid_report_out.mouse.scrollY);
        kbd_system.hid_report_out.mouse = outm;

        // info screen event
        if(new_key_press.sun) {
            if(lmoon) event = kbd_screen_event_CONFIG;
            else if(rmoon) event = kbd_screen_event_PREV;
            else event = kbd_screen_event_NEXT;
        } else if(new_key_press.backlight) {
            event = lmoon ? kbd_backlight_event_LOW : kbd_backlight_event_HIGH;
        }
    }

    return event;
}
