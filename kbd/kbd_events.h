#ifndef _KBD_EVENTS_H
#define _KBD_EVENTS_H

typedef enum {
    kbd_event_NONE = 0,

    // backlight adjust events
    kbd_backlight_event_HIGH,
    kbd_backlight_event_LOW,

    // screen events
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
} kbd_event_t;

#endif
