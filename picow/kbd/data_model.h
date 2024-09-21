#ifndef _DATA_MODEL_H
#define _DATA_MODEL_H

#include "hw_config.h"
#include "main.h"
#include "screen_model.h"
#include "tcp_server.h"
#include "util/pixel_anim.h"
#include "util/shared_buffer.h"

#ifdef KBD_NODE_AP
#include "key_layout.h"
#include "util/flash_store.h"
#endif

#ifdef KBD_NODE_LEFT
#include "util/rtc_model.h"
#endif

/*
 * Tasks (20 ms process cycle)
 *
 * core-0: scan key_press (left/right)            @ 5 ms
 *         BT send/recv (all)                     @ 10 ms
 *         led blinking (left/right)              @ no-delay
 *
 * core-1: scan tb_motion (right)                 @ 5 ms
 *         publish tb_motion (right)              @ 25 ms
 *         primary process                        @ 20 ms
 *         - update state using inputs (ap)
 *         - set task requests (ap)
 *         - make usb hid report and send (ap)
 *         execute task (left/right)              @ on demand
 *         process state->led (left/right)        @ no-delay
 *         process state->lcd (left)              @ 100 ms
 *         apply configs (all)                    @ on demand
 *         led pixels (left/right)                @ 50 ms
 *         read date-time & temperature (left)    @ 1 second
 *
 * It is critical to time the scan and processing for track ball
 * since it is an accumulator of dx and dy. We scan it quickly to avoid overflow of hardware registers.
 * And accumulate in memroy. Then use it at a lower frequency, i.e. 25 ms intervals. It should be
 * slower than our processing cycle which is 20 ms. So that, we can consume the readings without loss.
 *
 * Flow
 *
 * core-0
 * scan : left:key_scan  ==> left:key_press
 *        right:key_scan ==> right:key_press
 * BT   : left:key_press  ==> ap:left_key_press
 *        right:key_press ==> ap:right_key_press
 *        right:tb_motion ==> ap:tb_motion
 *        ap:system_state ==> left:system_state
 *        ap:system_state ==> right:system_state
 *        ap:left_task_request  ==> left:task_request
 *        ap:right_task_request ==> right:task_request
 *        left:task_response    ==> ap:left_task_response
 *        right:task_response   ==> ap:right_task_response
 *
 * core-1
 * scan   : right:tb_scan ==> right:tb_motion
 * Process: ap:left_key_press   ==>  ap:system_state
 *          ap:right_key_press       ap:hid_report_out
 *          ap:tb_motion
 *          ap:hid_report_in
 *          ap:left_task_response
 *          ap:right_task_response
 *
 *          ap:hid_report_out ==> ap:usb
 *          ap:usb ==> ap:hid_report_in
 *
 *          left:task_request  ==> left:task_response
 *          right:task_request ==> right:task_response
 *
 *          right:system_state ==> right:led (caps_lock)
 *          left:system_state  ==> left:lcd
 *                                 left:led (status)
 */

#define KBD_SB_COUNT 8

// The request/response should be large enough to fit 4 bytes header and 32 bytes data
// The data is 32 bytes so as to fit flash dataset which is also 32 bytes
// The header 4 bytes are 0:flag, 1:screen, 2:command, 3:(data size or config version)
#define KBD_TASK_SIZE 36
#define KBD_TASK_DATA_SIZE 32

typedef enum {
  kbd_comm_state_init = 0, // initial state waiting to handshake
  kbd_comm_state_ready,    // handshake done, ready to transfer data
  kbd_comm_state_data,     // active state, data transfer ongoing
  kbd_comm_state_reset     // wait till system is initialized back to init
} kbd_comm_state_t;

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

typedef struct {
  uint16_t cpi;
  uint8_t scroll_scale;
  uint8_t scroll_quad_weight;
  uint8_t delta_scale;
  uint8_t delta_quad_weight;
} kbd_tb_config_t;

typedef struct {
  uint32_t color;
  pixel_anim_style_t anim_style; // fixed, key press, fade etc
  uint8_t anim_cycles;           // number of cycles for a full transition (on to off)
} kbd_pixel_config_t;

typedef enum {
  kbd_led_state_OFF,
  kbd_led_state_ON,
  kbd_led_state_BLINK_LOW,    // 100(on), 900(off)
  kbd_led_state_BLINK_HIGH,   // 900, 100
  kbd_led_state_BLINK_SLOW,   // 1000, 1000
  kbd_led_state_BLINK_NORMAL, // 500, 500
  kbd_led_state_BLINK_FAST    // 100, 100
} kbd_led_state_t;

#define KBD_FLAG_CAPS_LOCK 0b10000000
#define KBD_FLAG_NUM_LOCK 0b01000000
#define KBD_FLAG_SCROLL_LOCK 0b00100000
#define KBD_FLAG_PIXELS_ON 0b00010000

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

#ifdef KBD_NODE_AP

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

#endif

/*
 * KBD System core1 data
 */

typedef struct {
#ifdef KBD_NODE_AP
  uint8_t left_key_press[hw_row_count];
  uint8_t right_key_press[hw_row_count];
#endif

#if defined(KBD_NODE_AP) || defined(KBD_NODE_RIGHT)
  uint64_t tb_motion_ts;
  kbd_tb_motion_t tb_motion;
#endif

#ifdef KBD_NODE_AP
  hid_report_in_t hid_report_in;   // incoming from usb host
  hid_report_out_t hid_report_out; // outgoing to usb host

  uint64_t left_task_request_ts;
  uint8_t left_task_request[KBD_TASK_SIZE];
  uint64_t left_task_response_ts;
  uint8_t left_task_response[KBD_TASK_SIZE];

  uint64_t right_task_request_ts;
  uint8_t right_task_request[KBD_TASK_SIZE];
  uint64_t right_task_response_ts;
  uint8_t right_task_response[KBD_TASK_SIZE];
#else
  uint64_t task_request_ts;
  uint8_t task_request[KBD_TASK_SIZE];
  uint64_t task_response_ts;
  uint8_t task_response[KBD_TASK_SIZE];
#endif

#ifdef KBD_NODE_AP
  uint64_t active_ts;   // timestamp of last activity (input_processor)
  uint8_t idle_minutes; // minutes it has been idle
#endif

#ifdef KBD_NODE_LEFT
  rtc_datetime_t date;
  uint8_t temperature;
#endif

  // ap - for processing, left - for display, right - for scanning
  kbd_tb_config_t tb_config;
  kbd_pixel_config_t pixel_config;

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
  uint32_t pixel_colors[hw_led_pixel_count];
#endif

#ifdef KBD_NODE_AP
  // flash loaded and saved on AP
  // the update is done via config screens (core1)
  flash_dataset_t *flash_datasets[KBD_CONFIG_SCREEN_COUNT];
  uint8_t left_flash_data_pos[KBD_CONFIG_SCREEN_COUNT];
  uint8_t right_flash_data_pos[KBD_CONFIG_SCREEN_COUNT];
#else
  // flash data as received from AP
  uint8_t flash_data[KBD_CONFIG_SCREEN_COUNT][KBD_TASK_DATA_SIZE];
  uint8_t flash_data_pos[KBD_CONFIG_SCREEN_COUNT];
#endif

  uint64_t state_ts;
  kbd_state_t state;
} kbd_system_core1_t;

/*
 * KBD System core0 data
 */

typedef struct {
  tcp_server_t tcp_server;

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
  uint8_t key_press[hw_row_count];
#endif
} kbd_system_core0_t;

/*
 * KBD System
 */

typedef struct {
#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
  volatile bool no_ap;
#endif

  volatile bool firmware_downloading;

  kbd_system_core0_t core0;
  kbd_system_core1_t core1;

  volatile bool pixels_on;
  volatile kbd_screen_t screen;
  volatile kbd_usb_hid_state_t usb_hid_state;
  volatile uint8_t backlight; // 0-100 %

  volatile kbd_comm_state_t comm_state[2]; // 0-left, 1-right

#ifdef KBD_NODE_AP
  volatile kbd_led_state_t led_left;  // left node indicator
  volatile kbd_led_state_t led_right; // right node indicator
#else
  volatile kbd_led_state_t led; // left: USB state, right: caps lock
#endif

  volatile kbd_led_state_t ledB; // board led: connection state

#ifdef KBD_NODE_AP
  volatile bool left_active;
  volatile bool right_active;
#else // left/right
  volatile bool ap_connected;
#endif

  volatile bool wifi; // enable wifi (for firmware download)

  shared_buffer_t *sb_state;

#ifdef KBD_NODE_AP
  shared_buffer_t *sb_left_key_press;
  shared_buffer_t *sb_right_key_press;
#endif

#ifdef KBD_NODE_AP
  shared_buffer_t *sb_left_task_request;
  shared_buffer_t *sb_left_task_response;
  shared_buffer_t *sb_right_task_request;
  shared_buffer_t *sb_right_task_response;
#else
  shared_buffer_t *sb_task_request;
  shared_buffer_t *sb_task_response;
#endif

#if defined(KBD_NODE_AP) || defined(KBD_NODE_RIGHT)
  shared_buffer_t *sb_tb_motion;
#endif

  volatile spin_lock_t *spin_lock; // for multicore data access
} kbd_system_t;

/*
 * Global vars
 */

extern kbd_system_t kbd_system;

/*
 * Model functions
 */

void init_data_model();

#endif
