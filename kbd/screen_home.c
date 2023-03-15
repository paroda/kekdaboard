#include "screen_processor.h"

bool render_screen_home(kbd_screen_event_t event) {
    return false;
}

// TODO create display request
// request-id 1 byte
// reqeust-type 4 bytes enum screen_home_display
// left_key_press 6 bytes
// right_key_press 6 bytes
// right_tb_motion 6 bytes
