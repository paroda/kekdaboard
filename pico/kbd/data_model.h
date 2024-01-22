#ifndef _DATA_MODEL_H
#define _DATA_MODEL_H

#include "hw_config.h"

#include "screen_model.h"
#include "peer_comm.h"
#include "rtc_model.h"
#include "key_layout.h"
#include "flash_store.h"
#include "pixel_anim.h"

/*
 * Tasks (10 ms process cycle)
 *
 * core-1: scan key_press @ 3 ms            core-0: scan tb_motion (right)                 @ 5 ms
 *         UART TX/RX     @ 3 ms                    publish tb_motion (right)              @ 20 ms
 *         led blinking   no-delay                  primary process                        @ 10 ms
 *                                                  - update state using inputs
 *                                                  - set task requests (master)
 *                                                  - make hid report and send via usb (master)
 *                                                  execute task (master & slave)          @ on demand
 *                                                  process state->led (left & right)      no-delay
 *                                                  process state->lcd (left)              @ 100 ms
 *                                                  apply configs                          @ on demand
 *                                                  read date-time & temperature           @ 1 second
 *                                                  update key switch leds                 @ 1 second
 *
 * It is critical to time the scan and processing for track ball
 * since it is an accumulator of dx and dy. We scan it quickly to avoid overflow of hardware registers.
 * And accumulate in memroy. Then use it at a lower frequency, i.e. 20 ms intervals. It should be
 * slower than our processing cycle which is 10 ms. So that, we can consume the readings without loss.
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
 * scan   : right:scan ==> right:right_tb_motion
 * Process: left:left_key_press   ==>  left:system_state
 *          left:right_key_press       left:hid_report_out
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

#define KBD_VERSION 0x05 // range x01 - 0x0F, for trivial match of both sides

#define KBD_SB_COUNT 8

#define KBD_TASK_REQUEST_SIZE 32
#define KBD_TASK_RESPONSE_SIZE 32

#define KBD_FLASH_HEADER_SIZE 256 // one flash page

// FLASH 1st page (256 bytes) (permanent)
// byes 0x00-0x0F 16bytes for my name
#define KBD_FLASH_NAME "Pradyumna"
// designate side (left: 0xF0, right:0xF1)
#define KBD_FLASH_ADDR_SIDE 0x10
#define KBD_FLASH_SIDE_LEFT 0xF0
#define KBD_FLASH_SIDE_RIGHT 0xF1

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
    int32_t dx;
    int32_t dy;
    uint8_t has_motion; // using uint8_t instead of bool
    uint8_t on_surface; // as bool may be larger
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

#define KBD_FLAG_CAPS_LOCK     0b10000000
#define KBD_FLAG_NUM_LOCK      0b01000000
#define KBD_FLAG_SCROLL_LOCK   0b00100000
#define KBD_FLAG_PIXELS_ON     0b00010000

typedef struct {
    // be careful about the size due to packing/alignment
    // order members larger to smaller
    // note that we are using memcmp to detect change
    // keep it small and simple
    uint8_t screen; // using uint8_t instead of enum, as enum may be larger
    uint8_t usb_hid_state;
    uint8_t flags;
    uint8_t backlight; // 0-100 %
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
    int16_t deltaX;
    int16_t deltaY;
    int16_t scrollX;
    int16_t scrollY;
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
    hid_report_out_keyboard_t keyboard;
    hid_report_out_mouse_t mouse;
    bool has_events;
} hid_report_out_t;

/*
 * KBD System
 */

typedef struct {
    uint8_t version;

    volatile kbd_side_t side;
    volatile kbd_role_t role;

    volatile bool ready;

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

    volatile uint8_t backlight; // 0-100 %
    volatile bool pixels_on;
    volatile kbd_screen_t screen;
    volatile kbd_usb_hid_state_t usb_hid_state;

    // local data

    uint64_t active_ts; // timestamp of last activity (input_processor), invalid on SLAVE
    uint8_t idle_minutes; // minutes of idleness to consider idle

    uint16_t tb_cpi;
    uint8_t tb_scroll_scale;
    uint8_t tb_scroll_quad_weight;
    uint8_t tb_delta_scale;
    uint8_t tb_delta_quad_weight;

    rtc_datetime_t date;
    uint8_t temperature;

    volatile kbd_led_state_t led;  // left: core0 system state, right: caps lock
    volatile kbd_led_state_t ledB; // left/right: core1 state

    volatile uint32_t pixel_color;
    volatile pixel_anim_style_t pixel_anim_style; // fixed, key press, fade etc
    volatile uint8_t pixel_anim_cycles; // number of cycles for a full transition (on to off)

    uint32_t pixel_colors_left[hw_led_pixel_count];
    uint32_t pixel_colors_right[hw_led_pixel_count];

    // flash loaded and saved on both sides, keeping them in sync
    // flash data is meant to be accessed by primary (core0) loop
    // the primary loop should maintain some more fields for others
    // the update is done via config screens which runs in primary loop
    uint8_t flash_header[KBD_FLASH_HEADER_SIZE];
    flash_dataset_t* flash_datasets[KBD_CONFIG_SCREEN_COUNT];

    peer_comm_config_t* comm;
    spin_lock_t* spin_lock;
} kbd_system_t;

/*
 * Global vars
 */

extern kbd_system_t kbd_system;

/*
 * Model functions
 */

void init_data_model();

void init_kbd_side(); // call after loading the flash_header

void set_kbd_role(kbd_role_t role);

#endif
