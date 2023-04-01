#include "pico/stdlib.h"

#include "screen_processor.h"

kbd_screen_t kbd_info_screens[KBD_INFO_SCREEN_COUNT] = {
    kbd_info_screen_welcome,
    kbd_info_screen_scan,
};

kbd_screen_t kbd_config_screens[KBD_CONFIG_SCREEN_COUNT] = {
    kbd_config_screen_date,
    kbd_config_screen_power,
    kbd_config_screen_tb,
};

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
    execute_screen_power,
    execute_screen_tb,
};

respond_screen_t* config_screen_responders[KBD_CONFIG_SCREEN_COUNT] = {
    respond_screen_date,
    respond_screen_power,
    respond_screen_tb,
};

init_config_screen_default_t* config_screen_default_initiators[KBD_CONFIG_SCREEN_COUNT] = {
    init_config_screen_default_date,
    init_config_screen_default_power,
    init_config_screen_default_tb,
};

apply_config_screen_t* config_screen_appliers[KBD_CONFIG_SCREEN_COUNT] = {
    apply_config_screen_date,
    apply_config_screen_power,
    apply_config_screen_tb,
};

uint8_t get_screen_index(kbd_screen_t screen) {
    bool config = screen & KBD_CONFIG_SCREEN_MASK;
    kbd_screen_t s0 = config ? kbd_config_screen_date : kbd_info_screen_welcome;
    return screen - s0;
}

bool is_nav_event(kbd_event_t event) {
    return event == kbd_screen_event_CONFIG
        || event == kbd_screen_event_EXIT
        || event == kbd_screen_event_NEXT
        || event == kbd_screen_event_PREV;
}

#define set_next_id(x) x=x==0xFF?0:x+1

void mark_left_request(kbd_screen_t screen) {
    set_next_id(kbd_system.left_task_request[0]);
    kbd_system.left_task_request[1] = screen;
    kbd_system.left_task_request_ts = time_us_64();
}

void mark_left_response() {
    kbd_system.left_task_response[0] = kbd_system.left_task_request[0];
    kbd_system.left_task_response[1] = kbd_system.left_task_request[1];
    kbd_system.left_task_response_ts = time_us_64();
}

void mark_right_request(kbd_screen_t screen) {
    set_next_id(kbd_system.right_task_request[0]);
    kbd_system.right_task_request[1] = screen;
    kbd_system.right_task_request_ts = time_us_64();
}

void mark_right_response() {
    kbd_system.right_task_response[0] = kbd_system.right_task_request[0];
    kbd_system.right_task_response[1] = kbd_system.right_task_request[1];
    kbd_system.right_task_response_ts = time_us_64();
}

static void execute_screen(kbd_event_t event) {
    bool config = kbd_system.screen & KBD_CONFIG_SCREEN_MASK;
    uint8_t si = get_screen_index(kbd_system.screen);

    (config ? config_screen_executors[si] : info_screen_executors[si])(event);
}

void execute_screen_processor(kbd_event_t event) {
    static kbd_event_t pending_event = kbd_event_NONE;
    event = event==kbd_event_NONE ? pending_event : event;

    uint8_t* lreq = kbd_system.left_task_request;
    uint8_t* lres = kbd_system.left_task_response;
    uint8_t* rreq = kbd_system.right_task_request;
    uint8_t* rres = kbd_system.right_task_response;

    if(lreq[0]!=lres[0] || rreq[0]!=rres[0]) {
        pending_event = event;
        return; // pending tasks
    }

    // execute the current screen, when not pending request
    execute_screen(event);
    pending_event = kbd_event_NONE;

    if(lreq[0]!=lres[0] || rreq[0]!=rres[0]) {
        if(is_nav_event(event)) pending_event = event;
        return; // no screen change when pending reqeust
    }

    // change screen if asked for
    if(is_nav_event(event)) {
        bool config = kbd_system.screen & KBD_CONFIG_SCREEN_MASK;
        uint8_t n = config ? KBD_CONFIG_SCREEN_COUNT : KBD_INFO_SCREEN_COUNT;
        kbd_screen_t s0 = config ? kbd_config_screen_date : kbd_info_screen_welcome;
        uint8_t si = kbd_system.screen - s0;

        kbd_screen_t screen = kbd_system.screen;
        if(event == kbd_screen_event_CONFIG && !config)
            screen = kbd_config_screen_date;
        else if(event == kbd_screen_event_EXIT && config)
            screen = kbd_info_screen_welcome;
        else if(event == kbd_screen_event_NEXT) {
            screen = si+1<n ? s0+si+1 : s0;
        } else if(event == kbd_screen_event_PREV) {
            screen = si>0 ? s0+si-1 : s0+n-1;
        }

        kbd_system.screen = screen;
        execute_screen(kbd_screen_event_INIT);
    }
}

static void respond_screen(kbd_screen_t screen) {
    bool config = screen & KBD_CONFIG_SCREEN_MASK;
    uint8_t si = get_screen_index(screen);
    (config ? config_screen_responders[si] : info_screen_responders[si])();
}

void respond_screen_processor(void) {
    uint8_t* req = kbd_system.side==kbd_side_LEFT ?
        kbd_system.left_task_request : kbd_system.right_task_request;
    uint8_t* res = kbd_system.side==kbd_side_LEFT ?
        kbd_system.left_task_response : kbd_system.right_task_response;

    if(req[0]==res[0]) return;

    respond_screen(req[1]);
}
