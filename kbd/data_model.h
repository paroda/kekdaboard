#ifndef _DATA_MODEL_H
#define _DATA_MODEL_H

#include "peer_comm.h"

/*
 * Tasks (10ms process cycle)
 *
 * core-1: scan key_press                   core-0: scan ball_scroll (right)
 *         UART TX/RX                               update state (master)
 *                                                  set tasks (master)
 *                                                  execute task (master & slave)
 *                                                  prepare hid report and send (master)
 *                                                  process state->led (left & right)
 *                                                  process state->lcd (left)
 *
 * Flow (left-master, right-slave)
 *
 * core-1
 * scan : left:scan ==> left:left_key_press
 *        right:scan ==> right:right_key_press
 * TX/RX: right:right_key_press ==> left:right_key_press
 *        right:right_ball_scroll ==> left:right_ball_scroll
 *        left:system_state ==> right:system_state
 *        left:right_task_requests ==> right:right_task_requests
 *        right:right_task_responses ==> left:right_task_responses
 *
 * core-0
 * scan   : right:scan ==> right:right_input->ball_scroll
 * Process: left:left_key_press   ==>  left:system_state
 *          left:right_key_press       left:hid_report
 *          left:right_ball_scroll
 *          left:usb_host_input
 *          left:left_task_responses
 *          left:right_task_responses
 *
 *          left:hid_report ==> left:usb
 *          left:usb ==> left:usb_host_input
 *
 *          left:left_task_requests ==> left:right_task_responses
 *          right:right_task_requests ==> right:right_task_responses
 *
 *          right:system_state ==> right:led (caps_lock)
 *          left:system_state ==> left:lcd
 *                                left:led (status)
 */

typedef enum {
    kbd_side_NONE = 0,
    kbd_side_LEFT = 1,
    kbd_side_RIGHT = 2
} kbd_side_t;

typedef enum {
    kbd_side_role_NONE = 0,
    kbd_side_role_MASTER = 1, // primary unit talking to the USB host
    kbd_side_role_SLAVE = 2  // secondary unit talking to the primary unit
} kbd_side_role_t;

/*
 * Global data
 */

typedef struct {
    kbd_side_t side;
    kbd_side_role_t side_role;

    bool ready;

    shared_buffer_t* left_key_press;
    shared_buffer_t* right_key_press;
    shared_buffer_t* right_ball_scroll;
    shared_buffer_t* system_state;
    shared_buffer_t* left_task_requests;
    shared_buffer_t* right_task_requests;
    shared_buffer_t* left_task_responses;
    shared_buffer_t* right_task_responses;

    peer_comm_config_t* peer_comm;
} kbd_system_t;

extern kbd_system_t kbd_system;

/*
 * Model functions
 */

void setup_data_model(uint8_t (*get) (void),
                      void (*put) (uint8_t),
                      uint64_t (*current_ts) (void));

void setup_role(kbd_side_role_t role);

#endif
