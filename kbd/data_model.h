#ifndef _DATA_MODEL_H
#define _DATA_MODEL_H

#include "hw_config.h"
#include "peer_comm.h"
#include "rtc_model.h"

/*
 * Tasks (10ms process cycle)
 *
 * core-1: scan key_press                   core-0: scan tb_motion (right)
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
 *        right:right_tb_motion ==> left:right_tb_motion
 *        left:system_state ==> right:system_state
 *        left:right_task_request ==> right:right_task_request
 *        right:right_task_response ==> left:right_task_response
 *
 * core-0
 * scan   : right:scan ==> right:right_input->tb_motion
 * Process: left:left_key_press   ==>  left:system_state
 *          left:right_key_press       left:hid_report
 *          left:right_tb_motion
 *          left:hid_report_in
 *          left:left_task_response
 *          left:right_task_response
 *
 *          left:hid_report_out ==> left:usb
 *          left:usb ==> left:hid_report_in
 *
 *          left:left_task_request ==> left:right_task_response
 *          right:right_task_request ==> right:right_task_response
 *
 *          right:system_state ==> right:led (caps_lock)
 *          left:system_state ==> left:lcd
 *                                left:led (status)
 */


#define KBD_SB_COUNT 8

#define KBD_TASK_REQUEST_SIZE 16
#define KBD_TASK_RESPONSE_SIZE 16

#define KBD_CONFIG_SCREEN_MASK 0x80
#define KBD_INFO_SCREEN_COUNT 1
#define KBD_CONFIG_SCREEN_COUNT 1

#define KEY_CODE_MAX 6 // same as in TinyUSB


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

typedef struct {
    // be careful about the size due to packing/alignment
    // order members larger to smaller
    bool has_motion;
    bool on_surface;
    int16_t dx;
    int16_t dy;
} kbd_tb_motion_t;

typedef enum {
    kbd_led_state_OFF,
    kbd_led_state_ON,
    kbd_led_state_BLINK_LOW,    // 100(on), 900(off)
    kbd_led_state_BLINK_HIGH,   // 900, 100
    kbd_led_state_BLINK_SLOW,   // 1000, 1000
    kbd_led_state_BLINK_NORMAL, // 500, 500
    kbd_led_state_BLINK_FAST    // 100, 100
} kbd_led_state_t;

typedef enum {
    kbd_info_screen_welcome = 0x00,
    kbd_info_screen_home = 0x01,
    kbd_config_screen_date = 0x80
} kbd_screen_t;

typedef enum {
    kbd_screen_event_NONE = 0,
    // only info screen
    kbd_screen_event_CONFIG, // SUN+L.MOON
    // only config screen
    kbd_screen_event_UP,     // ArrowUp
    kbd_screen_event_DOWN,   // ArrowDown
    kbd_screen_event_LEFT,   // ArrowLeft
    kbd_screen_event_RIGHT,  // ArrowRight
    kbd_screen_event_SEL_NEXT,  // Space
    kbd_screen_event_SEL_PREV,  // Space+L.MOON
    kbd_screen_event_SAVE,   // Enter
    kbd_screen_event_EXIT,   // Escape
    // both info and config screens
    kbd_screen_event_NEXT,   // SUN
    kbd_screen_event_PREV,   // SUN+R.MOON
} kbd_screen_event_t;

typedef struct {
    // be careful about the size due to packing/alignment
    // order members larger to smaller
    // note that we are using memcmp to detect change
    // keep it small and simple
    kbd_screen_t screen;
    bool caps_lock;
    bool num_lock;
    bool scroll_lock;
} kbd_state_t;

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
    uint8_t key_codes[KEY_CODE_MAX];
} hid_report_out_keyboard_t;

typedef struct {
    bool has_events;
    hid_report_out_mouse_t mouse;
    hid_report_out_keyboard_t keyboard;
} hid_report_out_t;

/*
 * KBD System
 */

typedef struct {
    volatile kbd_side_t side;
    volatile kbd_role_t role;

    volatile bool ready;

    rtc_datetime_t date;
    uint8_t temperature;

    uint64_t state_ts;
    kbd_state_t state;

    uint8_t left_key_press[KEY_ROW_COUNT];
    uint8_t right_key_press[KEY_ROW_COUNT];
    kbd_tb_motion_t right_tb_motion;

    hid_report_in_t hid_report_in;  // incoming from usb host
    hid_report_out_t hid_report_out; // outgoing to usb host

    uint64_t left_task_request_ts;
    uint8_t left_task_request[KBD_TASK_REQUEST_SIZE];

    uint64_t right_task_request_ts;
    uint8_t right_task_request[KBD_TASK_REQUEST_SIZE];

    uint64_t left_task_response_ts;
    uint8_t left_task_response[KBD_TASK_RESPONSE_SIZE];

    uint64_t right_task_response_ts;
    uint8_t right_task_response[KBD_TASK_RESPONSE_SIZE];

    /*
     * Processing:
     * sb_left_key_press      -->  sb_state
     * sb_right_key_press          sb_left_task_request
     * sb_right_tb_motion          sb_right_task_request
     * sb_left_task_response       hid_report_out
     * sb_right_task_response
     * hid_report_in
     */

    shared_buffer_t* sb_state;
    shared_buffer_t* sb_left_key_press;
    shared_buffer_t* sb_right_key_press;
    shared_buffer_t* sb_right_tb_motion;
    shared_buffer_t* sb_left_task_request;
    shared_buffer_t* sb_right_task_request;
    shared_buffer_t* sb_left_task_response;
    shared_buffer_t* sb_right_task_response;

    volatile kbd_usb_hid_state_t usb_hid_state;

    volatile kbd_led_state_t led;  // left: core0 system state, right: caps lock
    volatile kbd_led_state_t ledB; // left/right: core1 state

    peer_comm_config_t* comm;
} kbd_system_t;

/*
 * Global vars
 */

extern kbd_system_t kbd_system;

extern kbd_screen_t kbd_info_screens[KBD_INFO_SCREEN_COUNT];
extern kbd_screen_t kbd_config_screens[KBD_CONFIG_SCREEN_COUNT];

/*
 * Model functions
 */

void init_data_model();

void set_kbd_side(kbd_side_t side);

void set_kbd_role(kbd_role_t role);

#endif
