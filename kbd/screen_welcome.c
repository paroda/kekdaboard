#include "hw_model.h"
#include "screen_processor.h"

void lcd_display_body() {
    lcd_display_canvas(kbd_hw.lcd, 0, 40, kbd_hw.lcd_body);
}

void lcd_show_welcome() {
    lcd_canvas_clear(kbd_hw.lcd_body);
    // w:18x11=198 h:16, x:21-219
    lcd_canvas_text(kbd_hw.lcd_body, 21, 92, "Welcome Pradyumna!",
                    &lcd_font16, LCD_BODY_FG, LCD_BODY_BG);
    lcd_display_body();
}

void execute_screen_welcome(kbd_screen_event_t event) {
    if(event != kbd_screen_event_INIT) return;

    mark_left_request(kbd_info_screen_welcome);
}

void respond_screen_welcome(void) {
    lcd_show_welcome();

    mark_left_response();
}
