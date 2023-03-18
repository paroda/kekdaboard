#ifndef _SCREEN_MODEL_H
#define _SCREEN_MODEL_H

#include <stdint.h>

#define KBD_CONFIG_SCREEN_MASK 0x80

/*
 * To add a new screen
 *   increase the count
 *   add to enum
 *   declare the execute and respond functions in screen_processor.h
 *   add to screens, executors, responders lists in screen_processor.c
 *   implement them in screen_xxx.c
 *
 * Each config screen gets a flash storage of 32 bytes (kbd_system.flash_datasets)
 */

#define KBD_INFO_SCREEN_COUNT 2
#define KBD_CONFIG_SCREEN_COUNT 2

typedef enum {
    // info screens
    kbd_info_screen_welcome = 0x00,
    kbd_info_screen_scan,
    // config screens
    kbd_config_screen_date = 0x80,
    kbd_config_screen_power,
} kbd_screen_t;

typedef enum {
    kbd_screen_event_NONE = 0,
    kbd_screen_event_INIT,
    // only info screen
    kbd_screen_event_CONFIG, // SUN+L.MOON
    // only config screen
    kbd_screen_event_UP,     // ArrowUp
    kbd_screen_event_DOWN,   // ArrowDown
    kbd_screen_event_LEFT,   // ArrowLeft
    kbd_screen_event_RIGHT,  // ArrowRight
    kbd_screen_event_SEL_NEXT,  // Space
    kbd_screen_event_SEL_PREV,  // Space+L.MOON
    kbd_screen_event_SAVE,   // Enter
    kbd_screen_event_EXIT,   // Escape
    // both info and config screens
    kbd_screen_event_NEXT,   // SUN
    kbd_screen_event_PREV,   // SUN+R.MOON
} kbd_screen_event_t;

extern kbd_screen_t kbd_info_screens[KBD_INFO_SCREEN_COUNT];
extern kbd_screen_t kbd_config_screens[KBD_CONFIG_SCREEN_COUNT];

uint8_t get_screen_index(kbd_screen_t screen);

#endif
