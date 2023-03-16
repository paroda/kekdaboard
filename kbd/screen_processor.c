#include "screen_processor.h"

execute_screen_t* info_screen_executors[KBD_INFO_SCREEN_COUNT] = {
    execute_screen_welcome,
    execute_screen_scan,
};

respond_screen_t* info_screen_responders[KBD_INFO_SCREEN_COUNT] = {
    respond_screen_welcome,
    respond_screen_scan,
};

execute_screen_t* config_screen_executors[KBD_CONFIG_SCREEN_COUNT] = {
    execute_screen_date,
};

respond_screen_t* config_screen_responders[KBD_CONFIG_SCREEN_COUNT] = {
    respond_screen_date,
};

bool is_nav_event(kbd_screen_event_t event) {
    return event == kbd_screen_event_CONFIG
        || event == kbd_screen_event_EXIT
        || event == kbd_screen_event_NEXT
        || event == kbd_screen_event_PREV;
}

static void execute_screen(kbd_screen_event_t event) {
    bool config = kbd_system.state.screen & KBD_CONFIG_SCREEN_MASK;
    kbd_screen_t s0 = config ? kbd_config_screen_date : kbd_info_screen_welcome;
    uint8_t si = kbd_system.state.screen - s0;

    (config ? config_screen_executors[si] : info_screen_executors[si])(event);
}

void execute_screen_processor(kbd_screen_event_t event) {
    uint8_t* lreq = kbd_system.left_task_request;
    uint8_t* lres = kbd_system.left_task_response;
    uint8_t* rreq = kbd_system.left_task_request;
    uint8_t* rres = kbd_system.left_task_response;

    if((lreq[0]==0 || lres[0]!=0) && (rreq[0]==0 || rres[0]!=0)) {
        // execute the current screen, when not pending request
        execute_screen(event);
    }

    if(lreq[0]!=0 || rreq[0]!=0) return; // no screen change when pending reqeust

    // change screen if asked for
    if(is_nav_event(event)) {
        bool config = kbd_system.state.screen & KBD_CONFIG_SCREEN_MASK;
        uint8_t n = config ? KBD_CONFIG_SCREEN_COUNT : KBD_INFO_SCREEN_COUNT;
        kbd_screen_t s0 = config ? kbd_config_screen_date : kbd_info_screen_welcome;
        uint8_t si = kbd_system.state.screen - s0;

        kbd_screen_t screen = kbd_system.state.screen;
        if(event == kbd_screen_event_CONFIG && !config)
            screen = kbd_config_screen_date;
        else if(event == kbd_screen_event_EXIT && config)
            screen = kbd_info_screen_welcome;
        else if(event == kbd_screen_event_NEXT) {
            screen = si+1<n ? s0+si+1 : s0;
        } else if(event == kbd_screen_event_PREV) {
            screen = si>0 ? s0+si-1 : s0+n-1;
        }

        kbd_system.state.screen = screen;
        execute_screen(kbd_screen_event_INIT);
    }
}

static void respond_screen() {
    bool config = kbd_system.state.screen & KBD_CONFIG_SCREEN_MASK;
    kbd_screen_t s0 = config ? kbd_config_screen_date : kbd_info_screen_welcome;
    uint8_t si = kbd_system.state.screen - s0;

    (config ? config_screen_responders[si] : info_screen_responders[si])();
}

void respond_screen_processor(void) {
    uint8_t* req = kbd_system.side==kbd_side_LEFT ?
        kbd_system.left_task_request : kbd_system.right_task_request;
    uint8_t* res = kbd_system.side==kbd_side_LEFT ?
        kbd_system.left_task_response : kbd_system.right_task_response;

    if(req[0]==0 || res[0]!=0) return;

    respond_screen();
}
