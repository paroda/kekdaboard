#include "class/hid/hid.h"

#include "key_layout.h"

// Each entry is [modifier_mask, key_code, moon_key_code]
// moon_key_code==0 => same as key_code
const uint8_t key_layout[KEY_LAYOUT_ROW_COUNT][KEY_LAYOUT_COL_COUNT][3] = {
    /// Row 0
    {
        /// Col 0
        {0, HID_KEY_F1, 0},

        /// Col 1
        {0, HID_KEY_F2, 0},

        /// Col 2
        {0, HID_KEY_F3, 0},

        /// Col 3
        {0, HID_KEY_F4, 0},

        /// Col 4
        {0, HID_KEY_F5, 0},

        /// Col 5
        {0, HID_KEY_F6, 0},

        /// Col 6
        {0, HID_KEY_ESCAPE, 0},

        /// Col 7
        {0, HID_KEY_PRINT_SCREEN, 0},

        /// Col 8
        {0, HID_KEY_F7, 0},

        /// Col 9
        {0, HID_KEY_F8, 0},

        /// Col 10
        {0, HID_KEY_F9, 0},

        /// Col 11
        {0, HID_KEY_F10, 0},

        /// Col 12
        {0, HID_KEY_F11, 0},

        /// Col 13
        {0, HID_KEY_F12, 0},
    },

    /// Row 1
    {
        /// Col 0
        {0, HID_KEY_GRAVE, 0},

        /// Col 1
        {0, HID_KEY_1, 0},

        /// Col 2
        {0, HID_KEY_2, 0},

        /// Col 3
        {0, HID_KEY_3, 0},

        /// Col 4
        {0, HID_KEY_4, 0},

        /// Col 5
        {0, HID_KEY_5, 0},

        /// Col 6
        {0, KBD_KEY_SUN, 0}, // special key

        /// Col 7
        {0, HID_KEY_CAPS_LOCK, HID_KEY_NUM_LOCK},

        /// Col 8
        {0, HID_KEY_6, 0},

        /// Col 9
        {0, HID_KEY_7, 0},

        /// Col 10
        {0, HID_KEY_8, 0},

        /// Col 11
        {0, HID_KEY_9, 0},

        /// Col 12
        {0, HID_KEY_0, 0},

        /// Col 13
        {0, HID_KEY_MINUS, 0},
    },

    /// Row 2
    {
        /// Col 0
        {0, HID_KEY_TAB, 0},

        /// Col 1
        {0, HID_KEY_Q, 0},

        /// Col 2
        {0, HID_KEY_W, 0},

        /// Col 3
        {0, HID_KEY_E, 0},

        /// Col 4
        {0, HID_KEY_R, 0},

        /// Col 5
        {0, HID_KEY_T, 0},

        /// Col 6
        {0, HID_KEY_ARROW_UP, HID_KEY_PAGE_UP},

        /// Col 7
        {0, KBD_KEY_MOUSE_RIGHT, 0},

        /// Col 8
        {0, HID_KEY_Y, 0},

        /// Col 9
        {0, HID_KEY_U, 0},

        /// Col 10
        {0, HID_KEY_I, 0},

        /// Col 11
        {0, HID_KEY_O, 0},

        /// Col 12
        {0, HID_KEY_P, 0},

        /// Col 13
        {0, HID_KEY_BACKSLASH, 0},
    },

    /// Row 3
    {
        /// Col 0
        {0, HID_KEY_EQUAL, 0},

        /// Col 1
        {0, HID_KEY_A, 0},

        /// Col 2
        {0, HID_KEY_S, 0},

        /// Col 3
        {0, HID_KEY_D, 0},

        /// Col 4
        {0, HID_KEY_F, 0},

        /// Col 5
        {0, HID_KEY_G, 0},

        /// Col 6
        {0, HID_KEY_ARROW_DOWN, HID_KEY_PAGE_DOWN},

        /// Col 7
        {0, KBD_KEY_MOUSE_LEFT, KBD_KEY_MOUSE_MIDDLE},

        /// Col 8
        {0, HID_KEY_H, 0},

        /// Col 9
        {0, HID_KEY_J, 0},

        /// Col 10
        {0, HID_KEY_K, 0},

        /// Col 11
        {0, HID_KEY_L, 0},

        /// Col 12
        {0, HID_KEY_SEMICOLON, 0},

        /// Col 13
        {0, HID_KEY_APOSTROPHE, 0},
    },

    /// Row 4
    {
        /// Col 0
        {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_SHIFT_LEFT, 0},

        /// Col 1
        {0, HID_KEY_Z, 0},

        /// Col 2
        {0, HID_KEY_X, 0},

        /// Col 3
        {0, HID_KEY_C, 0},

        /// Col 4
        {0, HID_KEY_V, 0},

        /// Col 5
        {0, HID_KEY_B, 0},

        /// Col 6
        {0, HID_KEY_DELETE, 0},

        /// Col 7
        {0, HID_KEY_ENTER, 0},

        /// Col 8
        {0, HID_KEY_N, 0},

        /// Col 9
        {0, HID_KEY_M, 0},

        /// Col 10
        {0, HID_KEY_COMMA, 0},

        /// Col 11
        {0, HID_KEY_PERIOD, 0},

        /// Col 12
        {0, HID_KEY_SLASH, 0},

        /// Col 13
        {KEYBOARD_MODIFIER_RIGHTSHIFT, HID_KEY_SHIFT_RIGHT, 0},
    },

    /// Row 5
    {
        /// Col 0
        {KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_CONTROL_LEFT, 0},

        /// Col 1
        {KEYBOARD_MODIFIER_LEFTGUI, HID_KEY_GUI_LEFT, 0},

        /// Col 2
        {0, HID_KEY_ARROW_LEFT, HID_KEY_HOME},

        /// Col 3
        {0, HID_KEY_ARROW_RIGHT, HID_KEY_END},

        /// Col 4
        {0, HID_KEY_BACKSPACE, 0},

        /// Col 5
        {KEYBOARD_MODIFIER_LEFTALT, HID_KEY_ALT_LEFT, 0},

        /// Col 6
        {0, KBD_KEY_LEFT_MOON, 0},

        /// Col 7
        {0, KBD_KEY_RIGHT_MOON, 0},

        /// Col 8
        {KEYBOARD_MODIFIER_RIGHTALT, HID_KEY_ALT_RIGHT, 0},

        /// Col 9
        {0, HID_KEY_SPACE, 0},

        /// Col 10
        {0, HID_KEY_BRACKET_LEFT, 0},

        /// Col 11
        {0, HID_KEY_BRACKET_RIGHT, 0},

        /// Col 12
        // {0, HID_KEY_APPLICATION},
        {KEYBOARD_MODIFIER_RIGHTGUI, HID_KEY_GUI_RIGHT, 0},

        /// Col 13
        {KEYBOARD_MODIFIER_RIGHTCTRL, HID_KEY_CONTROL_RIGHT, 0},
    },
};
