#include "pico/stdlib.h"

#include "data_model.h"

static kbd_screen_t kbd_info_screens[KBD_INFO_SCREEN_COUNT] = {
    kbd_info_screen_welcome,
    kbd_info_screen_scan,
};

static kbd_screen_t kbd_config_screens[KBD_CONFIG_SCREEN_COUNT] = {
    kbd_config_screen_date,
    kbd_config_screen_power,
    kbd_config_screen_tb,
    kbd_config_screen_pixel,
};

#ifdef KBD_NODE_AP

screen_event_handler_t* handle_screen_event_welcome;
screen_event_handler_t* handle_screen_event_scan;

static screen_event_handler_t* info_screen_event_handlers[KBD_INFO_SCREEN_COUNT] = {
    handle_screen_event_welcome,
    handle_screen_event_scan,
};

screen_event_handler_t* handle_screen_event_date;
screen_event_handler_t* handle_screen_event_power;
screen_event_handler_t* handle_screen_event_tb;
screen_event_handler_t* handle_screen_event_pixel;

static screen_event_handler_t* config_screen_event_handlers[KBD_CONFIG_SCREEN_COUNT] = {
    handle_screen_event_date,
    handle_screen_event_power,
    handle_screen_event_tb,
    handle_screen_event_pixel,
};

#endif

screen_task_worker_t* work_screen_task_welcome;
screen_task_worker_t* work_screen_task_scan;

static screen_task_worker_t* info_screen_task_workers[KBD_INFO_SCREEN_COUNT] = {
    work_screen_task_welcome,
    work_screen_task_scan,
};

screen_task_worker_t* work_screen_task_date;
screen_task_worker_t* work_screen_task_power;
screen_task_worker_t* work_screen_task_tb;
screen_task_worker_t* work_screen_task_pixel;

static screen_task_worker_t* config_screen_task_workers[KBD_CONFIG_SCREEN_COUNT] = {
    work_screen_task_date,
    work_screen_task_power,
    work_screen_task_tb,
    work_screen_task_pixel,
};

config_screen_data_initiator_t* init_config_screen_data_date;
config_screen_data_initiator_t* init_config_screen_data_power;
config_screen_data_initiator_t* init_config_screen_data_tb;
config_screen_data_initiator_t* init_config_screen_data_pixel;

static config_screen_data_initiator_t* config_screen_data_initiators[KBD_CONFIG_SCREEN_COUNT] = {
    init_config_screen_data_date,
    init_config_screen_data_power,
    init_config_screen_data_tb,
    init_config_screen_data_pixel,
};

config_screen_data_applier_t* apply_config_screen_data_date;
config_screen_data_applier_t* apply_config_screen_data_power;
config_screen_data_applier_t* apply_config_screen_data_tb;
config_screen_data_applier_t* apply_config_screen_data_pixel;

static config_screen_data_applier_t* config_screen_data_appliers[KBD_CONFIG_SCREEN_COUNT] = {
    apply_config_screen_data_date,
    apply_config_screen_data_power,
    apply_config_screen_data_tb,
    apply_config_screen_data_pixel,
};

#ifdef KBD_NODE_AP

config_screen_data_getter_t get_config_screen_data_date;
config_screen_data_getter_t get_config_screen_data_power;
config_screen_data_getter_t get_config_screen_data_tb;
config_screen_data_getter_t get_config_screen_data_pixel;

config_screen_data_getter_t config_screen_data_getters[KBD_CONFIG_SCREEN_COUNT] = {
    get_config_screen_data_date,
    get_config_screen_data_power,
    get_config_screen_data_tb,
    get_config_screen_data_pixel,
};

#else

config_screen_data_setter_t set_config_screen_data_date;
config_screen_data_setter_t set_config_screen_data_power;
config_screen_data_setter_t set_config_screen_data_tb;
config_screen_data_setter_t set_config_screen_data_pixel;

config_screen_data_setter_t config_screen_data_setters[KBD_CONFIG_SCREEN_COUNT] = {
    set_config_screen_data_date,
    set_config_screen_data_power,
    set_config_screen_data_tb,
    set_config_screen_data_pixel,
};

#endif

////////////////////////////////////////////////////////////

bool is_nav_event(kbd_event_t event) {
    return event == kbd_screen_event_CONFIG
        || event == kbd_screen_event_EXIT
        || event == kbd_screen_event_NEXT
        || event == kbd_screen_event_PREV;
}

#define set_next_id(x) x=x==0xFF?0:x+1

void init_task_request(uint8_t* task_request, uint64_t* task_request_ts, kbd_screen_t screen) {
    set_next_id(task_request[0]);
    task_request[1] = screen;
    *task_request_ts = time_us_64();
}

void init_task_response(uint8_t* task_response, uint64_t* task_response_ts, uint8_t* task_request) {
    task_response[0] = task_request[0];
    task_response[1] = task_request[1];
    *task_response_ts = time_us_64();
}

#ifdef KBD_NODE_AP
void handle_screen_event(kbd_event_t event) {
    uint8_t* req = kbd_system.core0.task_request;
    uint8_t* res = kbd_system.core0.task_response;
    uint8_t* lreq = kbd_system.core0.left_task_request;
    uint8_t* lres = kbd_system.core0.left_task_response;
    uint8_t* rreq = kbd_system.core0.right_task_request;
    uint8_t* rres = kbd_system.core0.right_task_response;

    if(req[0]!=res[0] || lreq[0]!=lres[0] || rreq[0]!=rres[0]) {
        return; // pending tasks
    }

    kbd_screen_t screen = kbd_system.screen;
    bool config = screen & KBD_CONFIG_SCREEN_MASK;
    uint8_t n = config ? KBD_CONFIG_SCREEN_COUNT : KBD_INFO_SCREEN_COUNT;
    kbd_screen_t s0 = config ? kbd_config_screen_date : kbd_info_screen_welcome;
    uint8_t si = screen - s0;

    if(is_nav_event(event)) { // switch to screen init event if nav event
        if(event == kbd_screen_event_CONFIG && !config) {
            config = true;
            s0 = kbd_config_screen_date;
            si = 0;
        }
        else if(event == kbd_screen_event_EXIT && config) {
            config = false;
            s0 = kbd_info_screen_welcome;
            si = 0;
        }
        else if(event == kbd_screen_event_NEXT) {
            si = si+1<n ? si+1 : 0;
        } else if(event == kbd_screen_event_PREV) {
            si = si>0 ? si-1 : n-1;
        }
        screen = s0+si;

        kbd_system.screen = screen;
        event = kbd_screen_event_INIT;
    }

    (config ? config_screen_event_handlers[si] : info_screen_event_handlers[si])(event);
}
#endif

void work_screen_task() {
    uint8_t* req = kbd_system.task_request;
    uint8_t* res = kbd_system.task_response;

    if(req[0]==res[0]) return;

    kbd_screen_t screen = req[1];
    bool config = screen & KBD_CONFIG_SCREEN_MASK;
    kbd_screen_t s0 = config ? kbd_config_screen_date : kbd_info_screen_welcome;
    uint8_t si = screen - s0;
    (config ? config_screen_responders[si] : info_screen_responders[si])();
}

void init_config_screen_data() {
    for(uint i=0; i<KBD_CONFIG_SCREEN_COUNT; i++)
        config_screen_data_initiators[i]();
}

void apply_config_screen_data() {
    for(uint i=0; i<KBD_CONFIG_SCREEN_COUNT; i++)
        config_screen_data_appliers[i]();
}

#ifdef KBD_NODE_AP

void publish_config_screen_data(shared_buffer_t* sb) {
    static uint8_t si = 0;
    static uint8_t vs[KBD_CONFIG_SCREEN_COUNT] = {0}; // version counter
    // use two additional bytes for screen index and version
    uint8_t buf[FLASH_DATASET_SIZE+2];
    si = (si+1) < KBD_CONFIG_SCREEN_COUNT ? si+1 : 0;
    // skip if not applicable to current screen
    if(!config_screen_data_getters[si](buf+2)) return;
    buf[0] = si;
    buf[1] = vs[si] = vs[si]+1;
    write_shared_buffer(sb, time_us_64(), buf);
}

#else

void receive_config_screen_data(shared_buffer_t* sb) {
    static uint64_t last_ts = 0;
    static uint8_t vs[KBD_CONFIG_SCREEN_COUNT] = {0}; // version counter
    // use tow additional bytes for screen index and version
    uint8_t buf[FLASH_DATASET_SIZE+2];
    uint64_t ts;
    read_shared_buffer(sb, &ts, buf);
    if(ts==last_ts) return;
    last_ts = ts;
    uint8_t si = buf[0];
    if(buf[1]==vs[si]) return;
    vs[si] = buf[1];
    config_screen_data_setters[si](buf+2);
}

#endif
