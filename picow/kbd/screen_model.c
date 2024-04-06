#include <string.h>

#include "pico/stdlib.h"

#include "data_model.h"

kbd_screen_t kbd_info_screens[KBD_INFO_SCREEN_COUNT] = {
    kbd_info_screen_welcome,
    kbd_info_screen_scan,
};

kbd_screen_t kbd_config_screens[KBD_CONFIG_SCREEN_COUNT] = {
    kbd_config_screen_date,
    kbd_config_screen_power,
    kbd_config_screen_tb,
    kbd_config_screen_pixel,
};

#ifdef KBD_NODE_AP

extern screen_event_handler_t handle_screen_event_welcome;
extern screen_event_handler_t handle_screen_event_scan;

static screen_event_handler_t* info_screen_event_handlers[KBD_INFO_SCREEN_COUNT] = {
    handle_screen_event_welcome,
    handle_screen_event_scan,
};

extern screen_event_handler_t handle_screen_event_date;
extern screen_event_handler_t handle_screen_event_power;
extern screen_event_handler_t handle_screen_event_tb;
extern screen_event_handler_t handle_screen_event_pixel;

static screen_event_handler_t* config_screen_event_handlers[KBD_CONFIG_SCREEN_COUNT] = {
    handle_screen_event_date,
    handle_screen_event_power,
    handle_screen_event_tb,
    handle_screen_event_pixel,
};

#endif

extern screen_task_worker_t work_screen_task_welcome;
extern screen_task_worker_t work_screen_task_scan;

static screen_task_worker_t* info_screen_task_workers[KBD_INFO_SCREEN_COUNT] = {
    work_screen_task_welcome,
    work_screen_task_scan,
};

extern screen_task_worker_t work_screen_task_date;
extern screen_task_worker_t work_screen_task_power;
extern screen_task_worker_t work_screen_task_tb;
extern screen_task_worker_t work_screen_task_pixel;

static screen_task_worker_t* config_screen_task_workers[KBD_CONFIG_SCREEN_COUNT] = {
    work_screen_task_date,
    work_screen_task_power,
    work_screen_task_tb,
    work_screen_task_pixel,
};

extern config_screen_data_initiator_t init_config_screen_data_date;
extern config_screen_data_initiator_t init_config_screen_data_power;
extern config_screen_data_initiator_t init_config_screen_data_tb;
extern config_screen_data_initiator_t init_config_screen_data_pixel;

static config_screen_data_initiator_t* config_screen_data_initiators[KBD_CONFIG_SCREEN_COUNT] = {
    init_config_screen_data_date,
    init_config_screen_data_power,
    init_config_screen_data_tb,
    init_config_screen_data_pixel,
};

extern config_screen_data_applier_t apply_config_screen_data_date;
extern config_screen_data_applier_t apply_config_screen_data_power;
extern config_screen_data_applier_t apply_config_screen_data_tb;
extern config_screen_data_applier_t apply_config_screen_data_pixel;

static config_screen_data_applier_t* config_screen_data_appliers[KBD_CONFIG_SCREEN_COUNT] = {
    apply_config_screen_data_date,
    apply_config_screen_data_power,
    apply_config_screen_data_tb,
    apply_config_screen_data_pixel,
};

////////////////////////////////////////////////////////////

inline bool is_config_screen(kbd_screen_t screen) {
    return screen & KBD_CONFIG_SCREEN;
}

uint8_t get_screen_index(kbd_screen_t screen) {
    bool config = is_config_screen(screen);
    kbd_screen_t* screens = config ? kbd_config_screens : kbd_info_screens;
    uint8_t count = config ? KBD_CONFIG_SCREEN_COUNT : KBD_INFO_SCREEN_COUNT;
    for(uint8_t i=0; i<count; i++)
        if(screens[i]==screen) return i;

    return 0xFF; // invalid index
}

bool is_nav_event(kbd_event_t event) {
    return event == kbd_screen_event_CONFIG
        || event == kbd_screen_event_EXIT
        || event == kbd_screen_event_NEXT
        || event == kbd_screen_event_PREV;
}

static uint8_t next_task_id() {
    // 0 means no request, valid request id can be 1-255
    static uint8_t last_id = 0;
    return last_id = last_id < 0xFF ? last_id+1 : 1;
}

void init_task_request(uint8_t* task_request, uint64_t* task_request_ts, kbd_screen_t screen) {
    task_request[0] = next_task_id();
    task_request[1] = screen;
    task_request[2] = task_request[3] = 0; // init with no command/data
    *task_request_ts = time_us_64();
}

void init_task_response(uint8_t* task_response, uint64_t* task_response_ts, uint8_t* task_request) {
    task_response[0] = task_request[0];
    task_response[1] = task_request[1];
    task_response[2] = task_response[3] = 0; // init with no command/data
    *task_response_ts = time_us_64();
}

#ifdef KBD_NODE_AP

void handle_screen_event(kbd_event_t event) {
    uint8_t* req = kbd_system.core1.task_request;
    uint8_t* res = kbd_system.core1.task_response;
    uint8_t* lreq = kbd_system.core1.left_task_request;
    uint8_t* lres = kbd_system.core1.left_task_response;
    uint8_t* rreq = kbd_system.core1.right_task_request;
    uint8_t* rres = kbd_system.core1.right_task_response;

    if((req[0] && req[0]!=res[0])  // check req[0] for comm reset scenario
       || (lreq[0] && lreq[0]!=lres[0])
       || (rreq[0] && rreq[0]!=rres[0])) {
        return; // pending tasks
    }

    kbd_screen_t screen = kbd_system.screen;
    bool config = is_config_screen(screen);
    uint8_t n = config ? KBD_CONFIG_SCREEN_COUNT : KBD_INFO_SCREEN_COUNT;
    uint8_t si = get_screen_index(screen);

    static uint8_t res_last_id = 0;
    static uint8_t lres_last_id = 0;
    static uint8_t rres_last_id = 0;

    if((res[0] && res[0]!=res_last_id)  // check res[0] for comm reset scenario
       || (lres[0] && lres[0]!=lres_last_id)
       || (rres[0] && rres[0]!=rres_last_id)) {
        // process response command for this screen
        if((res[0] && res[1]==screen)
           || (lres[0] && lres[1]==screen)
           || (rres[0] && rres[1]==screen))
            event = kbd_screen_event_RESPONSE;
        res_last_id = res[0];
        lres_last_id = lres[0];
        rres_last_id = rres[0];
    }

    if(is_nav_event(event)) { // switch to screen init event if nav event
        if(event == kbd_screen_event_CONFIG && !config) {
            config = true;
            si = 0;
        }
        else if(event == kbd_screen_event_EXIT && config) {
            config = false;
            si = 0;
        }
        else if(event == kbd_screen_event_NEXT) {
            si = si+1<n ? si+1 : 0;
        } else if(event == kbd_screen_event_PREV) {
            si = si>0 ? si-1 : n-1;
        }
        screen = config ? kbd_config_screens[si] : kbd_info_screens[si];

        kbd_system.screen = screen;
        event = kbd_screen_event_INIT;
    }

    (config ? config_screen_event_handlers[si] : info_screen_event_handlers[si])(event);
}

#endif

void work_screen_task() {
    kbd_system_core1_t* c = &kbd_system.core1;
    uint8_t* req = c->task_request;
    uint8_t* res = c->task_response;

    if(!req[0] || res[0]==req[0]) return;

    kbd_screen_t screen = req[1];
    bool config = is_config_screen(screen);
    uint8_t si = get_screen_index(screen);

    init_task_response(res, &c->task_response_ts, req);

    (config ? config_screen_task_workers[si] : info_screen_task_workers[si])();
}

void init_config_screen_data() {
    for(uint i=0; i<KBD_CONFIG_SCREEN_COUNT; i++)
        config_screen_data_initiators[i]();
}

void apply_config_screen_data() {
    kbd_system_core1_t* c = &kbd_system.core1;
    static uint8_t applied[KBD_CONFIG_SCREEN_COUNT] = {0};
    for(uint8_t si=0; si<KBD_CONFIG_SCREEN_COUNT; si++) {
#ifdef KBD_NODE_AP
        flash_dataset_t* fd = c->flash_datasets[si];
        if(applied[si] != fd->pos) {
            config_screen_data_appliers[si]();
            applied[si] = fd->pos;
        }
#else
        if(applied[si] != c->flash_data_pos[si]) {
            config_screen_data_appliers[si]();
            applied[si] = c->flash_data_pos[si];
        }
#endif
    }
}
