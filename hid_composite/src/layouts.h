#ifndef _KBD_LAYOUTS_H_
#define _KBD_LAYOUTS_H_

#include "class/hid/hid.h"
#include "hardware/gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Keyboard Layouts - one entry for each layer */

    /// Keyboard Layers
    typedef enum
    {
        KBD_LAYER_BASE = 0, /// base layer
        KBD_LAYER_NUM,      /// Num Layer activated
        KBD_LAYER_COUNT
    } kbd_layer;

/* Layout size */
#define KBD_ROW_COUNT 7
#define KBD_COL_COUNT 14

    /* GPIO OUT pin for CapsLock LED */
    const uint kbd_led_capslock_gpio = 28;
    /* GPIO OUT pin for NumLock LED */
    const uint kbd_led_numlock_gpio = 27;
    /* GPIO OUT pin for NumLayer LED */
    const uint kbd_led_numlayer_gpio = 26;

    /* GPIO OUT pin for row */
    const uint kbd_layout_row_gpio[KBD_ROW_COUNT] = {16, 17, 18, 19, 20, 21, 22};

    /* GPIO IN pin for col */
    const uint kbd_layout_col_gpio[KBD_COL_COUNT] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2};

/* Special keys 0xF1 - 0xFF */
#define KBD_KEY_NUM_LAYER 0xF1

    /* Each entry is [modifier_mask, key_code] */
    const uint8_t kbd_layouts[KBD_LAYER_COUNT][KBD_ROW_COUNT][KBD_COL_COUNT][2] = {
        /* Base Layer */
        {
            /// Row 0
            {
                /// Col 0
                {0, HID_KEY_F1},

                /// Col 1
                {0, HID_KEY_F2},

                /// Col 2
                {0, HID_KEY_F3},

                /// Col 3
                {0, HID_KEY_F4},

                /// Col 4
                {0, HID_KEY_F5},

                /// Col 5
                {0, HID_KEY_F6},

                /// Col 6
                {0, HID_KEY_VOLUME_UP},

                /// Col 7
                {0, 0},

                /// Col 8
                {0, HID_KEY_F7},

                /// Col 9
                {0, HID_KEY_F8},

                /// Col 10
                {0, HID_KEY_F9},

                /// Col 11
                {0, HID_KEY_F10},

                /// Col 12
                {0, HID_KEY_F11},

                /// Col 13
                {0, HID_KEY_F12},
            },

            /// Row 1
            {
                /// Col 0
                {0, HID_KEY_EQUAL},

                /// Col 1
                {0, HID_KEY_1},

                /// Col 2
                {0, HID_KEY_2},

                /// Col 3
                {0, HID_KEY_3},

                /// Col 4
                {0, HID_KEY_4},

                /// Col 5
                {0, HID_KEY_5},

                /// Col 6
                {0, HID_KEY_MUTE},

                /// Col 7
                {0, 0},

                /// Col 8
                {0, HID_KEY_6},

                /// Col 9
                {0, HID_KEY_7},

                /// Col 10
                {0, HID_KEY_8},

                /// Col 11
                {0, HID_KEY_9},

                /// Col 12
                {0, HID_KEY_0},

                /// Col 13
                {0, HID_KEY_MINUS},
            },

            /// Row 2
            {
                /// Col 0
                {0, HID_KEY_TAB},

                /// Col 1
                {0, HID_KEY_Q},

                /// Col 2
                {0, HID_KEY_W},

                /// Col 3
                {0, HID_KEY_E},

                /// Col 4
                {0, HID_KEY_R},

                /// Col 5
                {0, HID_KEY_T},

                /// Col 6
                {0, HID_KEY_VOLUME_DOWN},

                /// Col 7
                {0, 0},

                /// Col 8
                {0, HID_KEY_Y},

                /// Col 9
                {0, HID_KEY_U},

                /// Col 10
                {0, HID_KEY_I},

                /// Col 11
                {0, HID_KEY_O},

                /// Col 12
                {0, HID_KEY_P},

                /// Col 13
                {0, HID_KEY_BACKSLASH},
            },

            /// Row 3
            {
                /// Col 0
                {0, HID_KEY_GRAVE},

                /// Col 1
                {0, HID_KEY_A},

                /// Col 2
                {0, HID_KEY_S},

                /// Col 3
                {0, HID_KEY_D},

                /// Col 4
                {0, HID_KEY_F},

                /// Col 5
                {0, HID_KEY_G},

                /// Col 6
                {0, HID_KEY_CAPS_LOCK},

                /// Col 7
                {0, KBD_KEY_NUM_LAYER}, /// special key - toggle num layer

                /// Col 8
                {0, HID_KEY_H},

                /// Col 9
                {0, HID_KEY_J},

                /// Col 10
                {0, HID_KEY_K},

                /// Col 11
                {0, HID_KEY_L},

                /// Col 12
                {0, HID_KEY_SEMICOLON},

                /// Col 13
                {0, HID_KEY_APOSTROPHE},
            },

            /// Row 4
            {
                /// Col 0
                {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_SHIFT_LEFT},

                /// Col 1
                {0, HID_KEY_Z},

                /// Col 2
                {0, HID_KEY_X},

                /// Col 3
                {0, HID_KEY_C},

                /// Col 4
                {0, HID_KEY_V},

                /// Col 5
                {0, HID_KEY_B},

                /// Col 6
                {0, HID_KEY_PRINT_SCREEN},

                /// Col 7
                {0, HID_KEY_NUM_LOCK},

                /// Col 8
                {0, HID_KEY_N},

                /// Col 9
                {0, HID_KEY_M},

                /// Col 10
                {0, HID_KEY_COMMA},

                /// Col 11
                {0, HID_KEY_PERIOD},

                /// Col 12
                {0, HID_KEY_SLASH},

                /// Col 13
                {KEYBOARD_MODIFIER_RIGHTSHIFT, HID_KEY_SHIFT_RIGHT},
            },

            /// Row 5
            {
                /// Col 0
                {0, HID_KEY_KEYPAD_ADD},

                /// Col 1
                {0, HID_KEY_KEYPAD_MULTIPLY},

                /// Col 2
                {0, HID_KEY_ARROW_LEFT},

                /// Col 3
                {0, HID_KEY_ARROW_RIGHT},

                /// Col 4
                {0, HID_KEY_SPACE},

                /// Col 5
                {0, HID_KEY_ESCAPE},

                /// Col 6
                {0, HID_KEY_HOME},

                /// Col 7
                {0, HID_KEY_PAGE_UP},

                /// Col 8
                {0, HID_KEY_INSERT},

                /// Col 9
                {0, HID_KEY_SPACE},

                /// Col 10
                {0, HID_KEY_ARROW_UP},

                /// Col 11
                {0, HID_KEY_ARROW_DOWN},

                /// Col 12
                {0, HID_KEY_BRACKET_LEFT},

                /// Col 13
                {0, HID_KEY_BRACKET_RIGHT},
            },

            /// Row 6
            {
                /// Col 0
                {0, HID_KEY_HOME},

                /// Col 1
                {0, HID_KEY_DELETE},

                /// Col 2
                {0, HID_KEY_BACKSPACE},

                /// Col 3
                {KEYBOARD_MODIFIER_LEFTGUI, HID_KEY_GUI_LEFT},

                /// Col 4
                {KEYBOARD_MODIFIER_LEFTALT, HID_KEY_ALT_LEFT},

                /// Col 5
                {KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_CONTROL_LEFT},

                /// Col 6
                {0, HID_KEY_END},

                /// Col 7
                {0, HID_KEY_PAGE_DOWN},

                /// Col 8
                {KEYBOARD_MODIFIER_RIGHTCTRL, HID_KEY_CONTROL_RIGHT},

                /// Col 9
                {KEYBOARD_MODIFIER_RIGHTALT, HID_KEY_ALT_RIGHT},

                /// Col 10
                {0, HID_KEY_APPLICATION},

                /// Col 11
                {0, HID_KEY_SPACE},

                /// Col 12
                {0, HID_KEY_ENTER},

                /// Col 13
                {0, HID_KEY_END},
            },
        },

        /* Num Layer */
        {
            /// Row 0
            {
                /// Col 0
                {0, HID_KEY_F1},

                /// Col 1
                {0, HID_KEY_F2},

                /// Col 2
                {0, HID_KEY_F3},

                /// Col 3
                {0, HID_KEY_F4},

                /// Col 4
                {0, HID_KEY_F5},

                /// Col 5
                {0, HID_KEY_F6},

                /// Col 6
                {0, HID_KEY_VOLUME_UP},

                /// Col 7
                {0, 0},

                /// Col 8
                {0, HID_KEY_F7},

                /// Col 9
                {0, HID_KEY_F8},

                /// Col 10
                {0, HID_KEY_F9},

                /// Col 11
                {0, HID_KEY_F10},

                /// Col 12
                {0, HID_KEY_F11},

                /// Col 13
                {0, HID_KEY_F12},
            },

            /// Row 1
            {
                /// Col 0
                {0, HID_KEY_EQUAL},

                /// Col 1
                {0, HID_KEY_1},

                /// Col 2
                {0, HID_KEY_2},

                /// Col 3
                {0, HID_KEY_3},

                /// Col 4
                {0, HID_KEY_4},

                /// Col 5
                {0, HID_KEY_5},

                /// Col 6
                {0, HID_KEY_MUTE},

                /// Col 7
                {0, 0},

                /// Col 8
                {0, HID_KEY_6},

                /// Col 9
                {0, HID_KEY_7},

                /// Col 10
                {0, HID_KEY_8},

                /// Col 11
                {0, HID_KEY_9},

                /// Col 12
                {0, HID_KEY_0},

                /// Col 13
                {0, HID_KEY_MINUS},
            },

            /// Row 2
            {
                /// Col 0
                {0, HID_KEY_TAB},

                /// Col 1
                {0, HID_KEY_Q},

                /// Col 2
                {0, HID_KEY_W},

                /// Col 3
                {0, HID_KEY_E},

                /// Col 4
                {0, HID_KEY_R},

                /// Col 5
                {0, HID_KEY_T},

                /// Col 6
                {0, HID_KEY_VOLUME_DOWN},

                /// Col 7
                {0, 0},

                /// Col 8
                {0, HID_KEY_Y},

                /// Col 9
                {0, HID_KEY_KEYPAD_7},

                /// Col 10
                {0, HID_KEY_KEYPAD_8},

                /// Col 11
                {0, HID_KEY_KEYPAD_9},

                /// Col 12
                {0, HID_KEY_KEYPAD_SUBTRACT},

                /// Col 13
                {0, HID_KEY_BACKSLASH},
            },

            /// Row 3
            {
                /// Col 0
                {0, HID_KEY_GRAVE},

                /// Col 1
                {0, HID_KEY_A},

                /// Col 2
                {0, HID_KEY_S},

                /// Col 3
                {0, HID_KEY_D},

                /// Col 4
                {0, HID_KEY_F},

                /// Col 5
                {0, HID_KEY_G},

                /// Col 6
                {0, HID_KEY_CAPS_LOCK},

                /// Col 7
                {0, KBD_KEY_NUM_LAYER}, /// special key - toggle num layer

                /// Col 8
                {0, HID_KEY_H},

                /// Col 9
                {0, HID_KEY_KEYPAD_4},

                /// Col 10
                {0, HID_KEY_KEYPAD_5},

                /// Col 11
                {0, HID_KEY_KEYPAD_6},

                /// Col 12
                {0, HID_KEY_KEYPAD_ADD},

                /// Col 13
                {0, HID_KEY_APOSTROPHE},
            },

            /// Row 4
            {
                /// Col 0
                {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_SHIFT_LEFT},

                /// Col 1
                {0, HID_KEY_Z},

                /// Col 2
                {0, HID_KEY_X},

                /// Col 3
                {0, HID_KEY_C},

                /// Col 4
                {0, HID_KEY_V},

                /// Col 5
                {0, HID_KEY_B},

                /// Col 6
                {0, HID_KEY_PRINT_SCREEN},

                /// Col 7
                {0, HID_KEY_NUM_LOCK},

                /// Col 8
                {0, HID_KEY_N},

                /// Col 9
                {0, HID_KEY_KEYPAD_1},

                /// Col 10
                {0, HID_KEY_KEYPAD_2},

                /// Col 11
                {0, HID_KEY_KEYPAD_3},

                /// Col 12
                {0, HID_KEY_KEYPAD_MULTIPLY},

                /// Col 13
                {KEYBOARD_MODIFIER_RIGHTSHIFT, HID_KEY_SHIFT_RIGHT},
            },

            /// Row 5
            {
                /// Col 0
                {0, HID_KEY_KEYPAD_ADD},

                /// Col 1
                {0, HID_KEY_KEYPAD_MULTIPLY},

                /// Col 2
                {0, HID_KEY_ARROW_LEFT},

                /// Col 3
                {0, HID_KEY_ARROW_RIGHT},

                /// Col 4
                {0, HID_KEY_SPACE},

                /// Col 5
                {0, HID_KEY_ESCAPE},

                /// Col 6
                {0, HID_KEY_HOME},

                /// Col 7
                {0, HID_KEY_PAGE_UP},

                /// Col 8
                {0, HID_KEY_INSERT},

                /// Col 9
                {0, HID_KEY_SPACE},

                /// Col 10
                {0, HID_KEY_KEYPAD_DECIMAL},

                /// Col 11
                {0, HID_KEY_KEYPAD_0},

                /// Col 12
                {0, HID_KEY_KEYPAD_ENTER},

                /// Col 13
                {0, HID_KEY_KEYPAD_DIVIDE},
            },

            /// Row 6
            {
                /// Col 0
                {0, HID_KEY_HOME},

                /// Col 1
                {0, HID_KEY_DELETE},

                /// Col 2
                {0, HID_KEY_BACKSPACE},

                /// Col 3
                {KEYBOARD_MODIFIER_LEFTGUI, HID_KEY_GUI_LEFT},

                /// Col 4
                {KEYBOARD_MODIFIER_LEFTALT, HID_KEY_ALT_LEFT},

                /// Col 5
                {KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_CONTROL_LEFT},

                /// Col 6
                {0, HID_KEY_END},

                /// Col 7
                {0, HID_KEY_PAGE_DOWN},

                /// Col 8
                {KEYBOARD_MODIFIER_RIGHTCTRL, HID_KEY_CONTROL_RIGHT},

                /// Col 9
                {KEYBOARD_MODIFIER_RIGHTALT, HID_KEY_ALT_RIGHT},

                /// Col 10
                {0, HID_KEY_APPLICATION},

                /// Col 11
                {0, HID_KEY_SPACE},

                /// Col 12
                {0, HID_KEY_ENTER},

                /// Col 13
                {0, HID_KEY_END},
            },
        },
    };

#ifdef __cplusplus
}
#endif

#endif /* _KBD_LAYOUTS_H_ */
