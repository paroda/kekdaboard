#include <string.h>

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

screen_event_handler_t handle_screen_event_welcome;
screen_event_handler_t handle_screen_event_scan;

static screen_event_handler_t* info_screen_event_handlers[KBD_INFO_SCREEN_COUNT] = {
    handle_screen_event_welcome,
    handle_screen_event_scan,
};

screen_event_handler_t handle_screen_event_date;
screen_event_handler_t handle_screen_event_power;
screen_event_handler_t handle_screen_event_tb;
screen_event_handler_t handle_screen_event_pixel;

static screen_event_handler_t* config_screen_event_handlers[KBD_CONFIG_SCREEN_COUNT] = {
    handle_screen_event_date,
    handle_screen_event_power,
    handle_screen_event_tb,
    handle_screen_event_pixel,
};

#endif

screen_task_worker_t work_screen_task_welcome;
screen_task_worker_t work_screen_task_scan;

static screen_task_worker_t* info_screen_task_workers[KBD_INFO_SCREEN_COUNT] = {
    work_screen_task_welcome,
    work_screen_task_scan,
};

screen_task_worker_t work_screen_task_date;
screen_task_worker_t work_screen_task_power;
screen_task_worker_t work_screen_task_tb;
screen_task_worker_t work_screen_task_pixel;

static screen_task_worker_t* config_screen_task_workers[KBD_CONFIG_SCREEN_COUNT] = {
    work_screen_task_date,
    work_screen_task_power,
    work_screen_task_tb,
    work_screen_task_pixel,
};

config_screen_data_initiator_t init_config_screen_data_date;
config_screen_data_initiator_t init_config_screen_data_power;
config_screen_data_initiator_t init_config_screen_data_tb;
config_screen_data_initiator_t init_config_screen_data_pixel;

static config_screen_data_initiator_t* config_screen_data_initiators[KBD_CONFIG_SCREEN_COUNT] = {
    init_config_screen_data_date,
    init_config_screen_data_power,
    init_config_screen_data_tb,
    init_config_screen_data_pixel,
};

config_screen_data_applier_t apply_config_screen_data_date;
config_screen_data_applier_t apply_config_screen_data_power;
config_screen_data_applier_t apply_config_screen_data_tb;
config_screen_data_applier_t apply_config_screen_data_pixel;

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

#define set_next_id(x) x=x==0xFF?0:x+1

void init_task_request(uint8_t* task_request, uint64_t* task_request_ts, kbd_screen_t screen) {
    set_next_id(task_request[0]);
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

static void publish_config() {
    kbd_system_core0_t* c = &kbd_system.core0;
    uint8_t* lreq = c->left_task_request;
    uint8_t* rreq = c->right_task_request;
    // sync config - send the config as a task request and iterate to next config screen
    static uint8_t si = 0;
    si = si+1<n ? si+1 : 0;
    kbd_screen_t screen = kbd_config_screens[si];
    init_task_request(lreq, &c->left_task_request_ts, screen);
    init_task_request(rreq, &c->right_task_request_ts, screen);
    flash_dataset_t* fd = c->flash_datasets[si];
    lreq[3] = rreq[3] = fd->pos;
    memcpy(lreq+4, fd->data, FLASH_DATASET_SIZE);
    memcpy(rreq+4, fd->data, FLASH_DATASET_SIZE);
}

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
    bool config = is_config_screen(screen);
    uint8_t n = config ? KBD_CONFIG_SCREEN_COUNT : KBD_INFO_SCREEN_COUNT;
    uint8_t si = get_screen_index(screen);

    if((res[1]==screen && res[2])
       || (lres[1]==screen && lres[2])
       || (rres[1]==screen && rres[2])) { // process response command for this screen
        event = kbd_screen_event_RESPONSE;
    } else if(is_nav_event(event)) { // switch to screen init event if nav event
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

    if(event==kbd_event_NONE && screen==kbd_info_screen_welcome) {
        publish_config();
    } else {
        (config ? config_screen_event_handlers[si] : info_screen_event_handlers[si])(event);
    }
}

#endif

void work_screen_task() {
    kbd_system_core0_t* c = &kbd_system.core0;
    uint8_t* req = c->task_request;
    uint8_t* res = c->task_response;

    if(req[0]==res[0]) return;

    kbd_screen_t screen = req[1];
    bool config = is_config_screen(screen);
    uint8_t si = get_screen_index(screen);

    init_task_response(res, &c->task_response_ts, req);

    if(config && req[2]==0) {
        if(req[3] != c->flash_data_version[si]) {
            c->flash_data_version = req[3];
            memcpy(c->flash_data[si], req+4, KBD_TASK_DATA_SIZE);
        }
    } else {
        (config ? config_screen_task_workers[si] : info_screen_task_workers[si])();
    }
}

void init_config_screen_data() {
    for(uint i=0; i<KBD_CONFIG_SCREEN_COUNT; i++)
        config_screen_data_initiators[i]();
}

void apply_config_screen_data() {
    kbd_system_core0_t* c = &kbd_system.core0;
    static uint8_t applied[KBD_CONFIG_SCREEN_COUNT] = {0};
    for(uint i=0; i<KBD_CONFIG_SCREEN_COUNT; i++) {
#ifdef KBD_NODE_AP
        flash_dataset_t* fd = c->flash_datasets[si];
        if(applied[si] != fd->pos) {
            config_screen_data_appliers[i]();
            applied[si] = fd->pos;
        }
#else
        if(applied[si] != c->flash_data_version[si]) {
            config_screen_data_appliers[i]();
            applied[si] = c->flash_data_version[si];
        }
#endif
    }
}
