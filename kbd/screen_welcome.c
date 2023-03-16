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
    uint8_t* req = kbd_system.left_task_request;
    uint8_t* res = kbd_system.left_task_response;
    if(res[0]!=0) req[0]=0;

    if(req[0]!=0 || is_nav_event(event)) return;

    if(event != kbd_screen_event_INIT) return;

    req[0] = 1;
    res[0] = 0;
}

void respond_screen_welcome(void) {
    uint8_t* req = kbd_system.left_task_request;
    uint8_t* res = kbd_system.left_task_response;
    if(req[0]==0) return;

    lcd_show_welcome();

    res[0] = 1;
}
