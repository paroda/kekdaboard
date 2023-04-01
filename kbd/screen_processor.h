#ifndef _SCREEN_PROCESSOR_H
#define _SCREEN_PROCESSOR_H

#include "data_model.h"

uint8_t get_screen_index(kbd_screen_t screen);

bool is_nav_event(kbd_event_t event);

void mark_left_request(kbd_screen_t screen);

void mark_left_response();

void mark_right_request(kbd_screen_t screen);

void mark_right_response();

// every screen renderer would render the body of the display
// display body location 0,41 - 240,240, size: 240x200

// screen is processed by master, but may have tasks to be done on the other side
// this is achieved by doing it in steps:
//   create request - by MASTER
//   create response - by appropriate SIDE
//   acknowledge response - by MASTER (must clear the request)

// each screen creates its own request and responds to it
// and it must clear the request upon completion

typedef void execute_screen_t (kbd_event_t event);

typedef void respond_screen_t(void);

typedef void init_config_screen_default_t();

typedef void apply_config_screen_t();

execute_screen_t execute_screen_processor;
respond_screen_t respond_screen_processor;

extern init_config_screen_default_t* config_screen_default_initiators[KBD_CONFIG_SCREEN_COUNT];
extern apply_config_screen_t* config_screen_appliers[KBD_CONFIG_SCREEN_COUNT];

/*
 * Info Screens
 */

execute_screen_t execute_screen_welcome;
respond_screen_t respond_screen_welcome;

execute_screen_t execute_screen_scan;
respond_screen_t respond_screen_scan;

/*
 * Config Screens
 */

execute_screen_t execute_screen_date;
respond_screen_t respond_screen_date;
init_config_screen_default_t init_config_screen_default_date;
apply_config_screen_t apply_config_screen_date;

execute_screen_t execute_screen_power;
respond_screen_t respond_screen_power;
init_config_screen_default_t init_config_screen_default_power;
apply_config_screen_t apply_config_screen_power;

execute_screen_t execute_screen_tb;
respond_screen_t respond_screen_tb;
init_config_screen_default_t init_config_screen_default_tb;
apply_config_screen_t apply_config_screen_tb;

#endif
