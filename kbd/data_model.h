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
    kbd_role_NONE = 0,
    kbd_role_MASTER = 1, // primary unit talking to the USB host
    kbd_role_SLAVE = 2  // secondary unit talking to the primary unit
} kbd_role_t;

typedef enum {
    kbd_usb_hid_state_UNMOUNTED = 0,
    kbd_usb_hid_state_MOUNTED = 1,
    kbd_usb_hid_state_SUSPENDED = 2
} kbd_usb_hid_state_t;

/*
 * HID report
 */

typedef struct {
    bool NumLock;
    bool CapsLock;
    bool ScrollLock;
    bool Compose;
    bool Kana;
} hid_report_in_keyboard_t;

typedef struct {
    hid_report_in_keyboard_t keyboard;
} hid_report_in_t;

typedef struct {
    bool left;
    bool right;
    bool middle;
    bool backward;
    bool forward;
    int8_t deltaX;
    int8_t deltaY;
    int8_t scrollX;
    int8_t scrollY;
} hid_report_out_mouse_t;

typedef struct {
    bool leftCtrl;
    bool leftShift;
    bool leftAlt;
    bool leftGui;
    bool rightCtrl;
    bool rightShift;
    bool rightAlt;
    bool rightGui;
    uint8_t key_codes[6];
} hid_report_out_keyboard_t;

typedef struct {
    bool has_events;
    hid_report_out_mouse_t mouse;
    hid_report_out_keyboard_t keyboard;
} hid_report_out_t;

/*
 * Global data
 */

typedef enum {
    kbd_led_state_ON = 0,
    kbd_led_state_OFF = 1,
    kbd_led_state_BLINK_LOW,    // 100, 900
    kbd_led_state_BLINK_HIGH,   // 900, 100
    kbd_led_state_BLINK_SLOW,   // 1000, 1000
    kbd_led_state_BLINK_NORMAL, // 500, 500
    kbd_led_state_BLINK_FAST    // 100, 100
} kbd_led_state_t;

typedef struct {
    volatile kbd_side_t side;
    volatile kbd_role_t role;

    volatile bool ready;

    shared_buffer_t* left_key_press;
    shared_buffer_t* right_key_press;
    shared_buffer_t* right_ball_scroll;
    shared_buffer_t* system_state;
    shared_buffer_t* left_task_request;
    shared_buffer_t* right_task_request;
    shared_buffer_t* left_task_response;
    shared_buffer_t* right_task_response;

    hid_report_in_t hid_report_in;  // incoming from usb host
    hid_report_out_t hid_report_out; // outgoing to usb host

    volatile kbd_usb_hid_state_t usb_hid_state;

    volatile kbd_led_state_t led;
    volatile kbd_led_state_t ledB;

    peer_comm_config_t* comm;
} kbd_system_t;

extern kbd_system_t kbd_system;

/*
 * Model functions
 */

void init_data_model();

void set_kbd_side(kbd_side_t side);

void set_kbd_role(kbd_role_t role);

#endif
