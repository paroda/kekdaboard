#include "hw_model.h"
#include "screen_processor.h"

void execute_screen_welcome(kbd_screen_event_t event) {
    if(event != kbd_screen_event_INIT) return;

    mark_left_request(kbd_info_screen_welcome);
}

void respond_screen_welcome(void) {
    lcd_show_welcome();

    mark_left_response();
}
