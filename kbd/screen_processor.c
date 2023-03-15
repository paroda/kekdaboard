#include "screen_processor.h"

render_screen_t* info_screen_renderer[KBD_INFO_SCREEN_COUNT] = {
    render_screen_home,
};

render_screen_t* config_screen_renderer[KBD_CONFIG_SCREEN_COUNT] = {
    render_screen_date,
};

bool execute_screen_processor(kbd_screen_event_t event) {

    if(kbd_system.state.screen==kbd_info_screen_welcome) {
        render_screen_home(event);
        return true;
    }

    bool config = kbd_system.state.screen & KBD_CONFIG_SCREEN_MASK;
    uint8_t si = kbd_system.state.screen - (config ? kbd_config_screen_date : kbd_info_screen_home);

    bool done = (config ? config_screen_renderer[si] : info_screen_renderer[si])(event);

    if(done) {
        // TODO get next/prev or config or current
        //      handle NEXT, PREV, CONFIG, EXIT events
    }

    return true;
}
