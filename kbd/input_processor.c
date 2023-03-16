#include <string.h>

#include "class/hid/hid.h"

#include "key_layout.h"
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

kbd_screen_event_t execute_input_processor() {
    const uint8_t* key_press[KEY_PRESS_MAX];
    uint8_t n = key_layout_read(key_press);

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

    hid_report_out_keyboard_t outk;
    memset(&outk, 0, sizeof(hid_report_out_keyboard_t));
    hid_report_out_mouse_t outm;
    memset(&outm, 0, sizeof(hid_report_out_mouse_t));

    bool k_up=false, k_down=false, k_left=false, k_right=false;
    bool k_space=false, k_enter=false, k_escape=false;

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
        switch(code) {
        case KBD_KEY_SUN:
        case KBD_KEY_LEFT_MOON:
        case KBD_KEY_RIGHT_MOON:
            break;
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
            switch(code) {
            case HID_KEY_ARROW_UP:
                k_up=true;
                break;
            case HID_KEY_ARROW_DOWN:
                k_down=true;
                break;
            case HID_KEY_ARROW_LEFT:
                k_left=true;
                break;
            case HID_KEY_ARROW_RIGHT:
                k_right=true;
                break;
            case HID_KEY_SPACE:
                k_space=true;
                break;
            case HID_KEY_ENTER:
                k_enter=true;
                break;
            case HID_KEY_ESCAPE:
                k_escape=true;
                break;
            default: // skip
                break;
            }
        }
    }

    if(kbd_system.right_tb_motion.has_motion) {
        if(moon) {
            outm.scrollX = kbd_system.right_tb_motion.dx/20;
            outm.scrollY = kbd_system.right_tb_motion.dy/20;
        } else {
            outm.deltaX = kbd_system.right_tb_motion.dx;
            outm.deltaY = kbd_system.right_tb_motion.dy;
        }
    }

    bool has_events = (n_key_codes>0
                       || outk.leftCtrl  || outk.leftShift  || outk.leftAlt  || outk.leftGui
                       || outk.rightCtrl || outk.rightShift || outk.rightAlt || outk.rightGui
                       || outm.left || outm.right || outm.middle || outm.backward || outm.forward
                       || kbd_system.right_tb_motion.has_motion);

    // note key press events for screen
    static bool sun_prev = false;
    bool sun_press = sun && !sun_prev;
    sun_prev = sun;

    static bool up_prev = false;
    bool up_press = k_up && !up_prev;
    up_prev = k_up;

    static bool down_prev = false;
    bool down_press = k_down && !down_prev;
    down_prev = k_down;

    static bool left_prev = false;
    bool left_press = k_left && ! left_prev;
    left_prev = k_left;

    static bool right_prev = false;
    bool right_press = k_right && !right_prev;
    right_prev = k_right;

    static bool space_prev = false;
    bool space_press = k_space && !space_prev;
    space_prev = k_space;

    static bool enter_prev = false;
    bool enter_press = k_enter && !enter_prev;
    enter_prev = k_enter;

    static bool escape_prev = false;
    bool escape_press = k_escape && !escape_prev;
    escape_prev = k_escape;

    kbd_screen_event_t screen_event = kbd_screen_event_NONE;

    bool config_mode = kbd_system.state.screen & KBD_CONFIG_SCREEN_MASK;
    if(config_mode) {
        memset(&kbd_system.hid_report_out, 0, sizeof(hid_report_out_t));

        // config screen event
        if(sun_press) {
            if(rmoon) screen_event = kbd_screen_event_NEXT;
            else screen_event = kbd_screen_event_PREV;
        } else if(up_press) {
            screen_event = kbd_screen_event_UP;
        } else if(down_press) {
            screen_event = kbd_screen_event_DOWN;
        } else if(left_press) {
            screen_event = kbd_screen_event_LEFT;
        } else if(right_press) {
            screen_event = kbd_screen_event_RIGHT;
        } else if(space_press) {
            screen_event = lmoon ? kbd_screen_event_PREV : kbd_screen_event_NEXT;
        } else if(enter_press) {
            screen_event = kbd_screen_event_SAVE;
        } else if(escape_press) {
            screen_event = kbd_screen_event_EXIT;
        }
    } else {
        kbd_system.hid_report_out.has_events = has_events;
        kbd_system.hid_report_out.keyboard = outk;
        kbd_system.hid_report_out.mouse = outm;

        // info screen event
        if(sun_press) {
            if(lmoon) screen_event = kbd_screen_event_CONFIG;
            else if(rmoon) screen_event = kbd_screen_event_PREV;
            else screen_event = kbd_screen_event_NEXT;
        }
    }

    return screen_event;
}
