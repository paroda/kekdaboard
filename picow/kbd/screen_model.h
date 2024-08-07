#ifndef _SCREEN_MODEL_H
#define _SCREEN_MODEL_H

#include <stdint.h>
#include <stdbool.h>

#include "kbd_events.h"
#include "util/shared_buffer.h"

#define KBD_CONFIG_SCREEN 0x80
#define KBD_SCREEN_ID_MASK 0x7F

/*
 * To add a new screen
 *   increase the count
 *   add to enum
 *   declare and add to screens, handlers, workers lists in screen_model.c
 *   implement them in screen/xxx.c
 *
 * Each config screen gets a flash storage of 32 bytes (kbd_system.core1.flash_datasets)
 */

#define KBD_INFO_SCREEN_COUNT 2
#define KBD_CONFIG_SCREEN_COUNT 4

typedef enum {
    // info screens
    kbd_info_screen_welcome = 0x00,
    kbd_info_screen_scan,
    // config screens
    kbd_config_screen_date = 0x80,
    kbd_config_screen_power,
    kbd_config_screen_tb,
    kbd_config_screen_pixel,
} kbd_screen_t;

extern kbd_screen_t kbd_info_screens[KBD_INFO_SCREEN_COUNT];
extern kbd_screen_t kbd_config_screens[KBD_CONFIG_SCREEN_COUNT];

bool is_config_screen(kbd_screen_t screen);
uint8_t get_screen_index(kbd_screen_t screen);

// every screen renderer would render the body of the display
// display body location 0,41 - 240,240, size: 240x200

// screen event is processed by AP node
// rendering is done by LEFT node, on receiving render task from AP Node
// updates to config is sent as task from AP node to LEFT/RIGHT node
// each task is acknowledged via a task response returned to AP node

// there is only one task possible at a time for any node.
// no more tasks until the current one is completed.

// each screen creates its own request and responds to it
// and it must clear the request upon completion

// the task and response flow is reset on connection betwen AP and LEFT/RIGHT node
// also when screen is switched, pending tasks are discarded

// check if the event is to switch screen
bool is_nav_event(kbd_event_t event);

// common initialization of the request/response data for any screen
void init_task_request(uint8_t* task_request, uint64_t* task_request_ts, kbd_screen_t screen);
void init_task_response(uint8_t* task_response, uint64_t* task_response_ts, uint8_t* task_request);

#ifdef KBD_NODE_AP
typedef void screen_event_handler_t(kbd_event_t event); // handler user raised events
screen_event_handler_t handle_screen_event;
#else
typedef void screen_task_worker_t(); // work on the screen tasks
screen_task_worker_t work_screen_task;
#endif

typedef void config_screen_data_initiator_t();
config_screen_data_initiator_t init_config_screen_data; // system data -> screen data -> flash data

typedef void config_screen_data_applier_t();
config_screen_data_applier_t apply_config_screen_data; // flash data -> screen data -> system data

#endif
