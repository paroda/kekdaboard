#ifndef _KEY_LAYOUT_H_
#define _KEY_LAYOUT_H_

#include <stdint.h>

#define KEY_ROW_COUNT 6
#define KEY_COL_COUNT 7
#define KEY_CODE_MAX 6 // same as in TinyUSB

// special keys
#define KBD_KEY_SUN            0xF1
#define KBD_KEY_LEFT_MOON      0xF2
#define KBD_KEY_RIGHT_MOON     0xF3
#define KBD_KEY_MOUSE_LEFT     0xF4
#define KBD_KEY_MOUSE_RIGHT    0xF5
#define KBD_KEY_MOUSE_MIDDLE   0xF6
#define KBD_KEY_MOUSE_BACKWARD 0xF7
#define KBD_KEY_MOUSE_FORWARD  0xF8
#define KBD_KEY_BACKLIGHT      0xF9
#define KBD_KEY_PIXELS         0xFA

#define KEY_LAYOUT_ROW_COUNT KEY_ROW_COUNT // 6
#define KEY_LAYOUT_COL_COUNT (KEY_COL_COUNT * 2) // 14 (7*2)

extern const uint8_t key_layout[KEY_LAYOUT_ROW_COUNT][KEY_LAYOUT_COL_COUNT][3];

#endif /* _KBD_LAYOUT_H_ */
