#ifndef _SCREEN_MODEL_H
#define _SCREEN_MODEL_H

#include <stdint.h>

#include "kbd_events.h"

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

extern kbd_screen_t kbd_info_screens[KBD_INFO_SCREEN_COUNT];
extern kbd_screen_t kbd_config_screens[KBD_CONFIG_SCREEN_COUNT];

uint8_t get_screen_index(kbd_screen_t screen);

#endif
