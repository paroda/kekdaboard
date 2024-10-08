#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"

#include "bsp/board_api.h"
#include "tusb.h"

#include "data_model.h"
#include "usb_descriptors.h"
#include "usb_hid.h"

static struct {
    uint32_t last_reset_ms;
    uint32_t mouse;
    uint32_t keyboard;
    uint32_t not_ready;
    uint32_t ms;
} tud_fail_counter;

static struct {
    volatile uint32_t values[8];
    // 0 - max loop counter for 50ms led task
    // 1 - comm_task count per second
    // 2 - max_poll_ms in previous 10 seconds
    // 3 - poll_ms (avg) in previous 10 seconds
    // 4 - poll_count in previous 10 seconds
} core0_debug;

void set_core0_debug(uint8_t index, uint32_t value) {
    core0_debug.values[index] = value;
}

static bool send_hid_gamepad_report() {
    // Not Used
    return true;
}

static bool send_hid_consumer_report() {
    // Not Used
    return true;
}

/// Custom HID Boot Protocol Mouse Report.
typedef struct TU_ATTR_PACKED {
    uint8_t buttons; /**< buttons mask for currently pressed buttons in the mouse. */
    int16_t  x;       /**< Current delta x movement of the mouse. */
    int16_t  y;       /**< Current delta y movement on the mouse. */
    int16_t  wheel;   /**< Current delta wheel movement on the mouse. */
    int16_t  pan;     // using AC Pan
} kbd_mouse_report_t;

static bool send_hid_mouse_report() {
    hid_report_out_mouse_t* m = &(kbd_system.core1.hid_report_out.mouse);

    uint8_t buttons = 0 |
        (m->left     ? MOUSE_BUTTON_LEFT     : 0) |
        (m->right    ? MOUSE_BUTTON_RIGHT    : 0) |
        (m->middle   ? MOUSE_BUTTON_MIDDLE   : 0) |
        (m->backward ? MOUSE_BUTTON_BACKWARD : 0) |
        (m->forward  ? MOUSE_BUTTON_FORWARD  : 0);

    kbd_mouse_report_t report = {
        .buttons = buttons,
        .x       = m->deltaX,
        .y       = m->deltaY,
        .wheel   = m->scrollY,
        .pan     = m->scrollX
    };

    bool success = false;
    if(tud_hid_n_report(ITF_NUM_HID1, REPORT_ID_MOUSE, &report, sizeof(report))) {
        m->deltaX = 0;
        m->deltaY = 0;
        m->scrollX = 0;
        m->scrollY = 0;
        success = true;
    }
    if(!success) tud_fail_counter.mouse++;
    return success;
}

static bool send_hid_keyboard_report() {
    hid_report_out_keyboard_t* k = &(kbd_system.core1.hid_report_out.keyboard);

    uint8_t modifiers = 0 |
        (k->leftCtrl   ? KEYBOARD_MODIFIER_LEFTCTRL   : 0) |
        (k->leftShift  ? KEYBOARD_MODIFIER_LEFTSHIFT  : 0) |
        (k->leftAlt    ? KEYBOARD_MODIFIER_LEFTALT    : 0) |
        (k->leftGui    ? KEYBOARD_MODIFIER_LEFTGUI    : 0) |
        (k->rightCtrl  ? KEYBOARD_MODIFIER_RIGHTCTRL  : 0) |
        (k->rightShift ? KEYBOARD_MODIFIER_RIGHTSHIFT : 0) |
        (k->rightAlt   ? KEYBOARD_MODIFIER_RIGHTALT   : 0) |
        (k->rightGui   ? KEYBOARD_MODIFIER_RIGHTGUI   : 0);

    bool success = tud_hid_n_keyboard_report(ITF_NUM_HID1, REPORT_ID_KEYBOARD, modifiers, k->key_codes);
    if(!success) tud_fail_counter.keyboard++;
    return success;
}

static bool send_hid_report(uint8_t report_id) {
    if(!tud_hid_n_ready(ITF_NUM_HID1)) {
        tud_fail_counter.not_ready++;
        return false;
    }

    // NOTE: you must send report for all previous reports in report id sequence
    //       otherwise your target report will never get called.
    //       For example, if you do not send the mouse report, then there would be
    //       no completion callback to send the next report which is consumer control.
    switch (report_id)
    {
    case REPORT_ID_KEYBOARD:
        return send_hid_keyboard_report();
    case REPORT_ID_MOUSE:
        return send_hid_mouse_report();
    case REPORT_ID_CONSUMER_CONTROL:
        return send_hid_consumer_report();
    case REPORT_ID_GAMEPAD:
        return send_hid_gamepad_report();
    default:
        return false;
    }
}

static void hid_task(void) {
    if(tud_suspended() && kbd_system.core1.hid_report_out.has_events) {
        // Wake up host if we are in suspended mode
        // and REMOTE_WAKEUP feature is enabled by host
        tud_remote_wakeup();
    } else {
        // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
        // Send the report chain on
        //  - new events (like keypress)
        //  - end of events (like release of keypress)
        static bool had_events = false;
        if(kbd_system.core1.hid_report_out.has_events) { // has some events
            send_hid_report(REPORT_ID_KEYBOARD);
            had_events = true;
        } else {
            // report end of previous envents
            if(had_events)
                if(send_hid_report(REPORT_ID_KEYBOARD))
                    had_events = false;
        }
    }
}

void usb_hid_idle_task(void) {
    tud_task();

    uint32_t dt = 10000; // 10 second interval
    tud_fail_counter.ms = board_millis();
    if(tud_fail_counter.ms >= tud_fail_counter.last_reset_ms+dt) {
        tud_fail_counter.keyboard = 0;
        tud_fail_counter.mouse = 0;
        tud_fail_counter.not_ready = 0;
        tud_fail_counter.last_reset_ms = tud_fail_counter.last_reset_ms==0 ?
            tud_fail_counter.ms : tud_fail_counter.last_reset_ms+dt;
    }
}

void usb_hid_task(void) {
    tud_task();
    hid_task();
}

void usb_hid_init(void) {
    board_init();

    // init device stack on configured roothub port
    tud_init(BOARD_TUD_RHPORT);

    if(board_init_after_tusb) {
        board_init_after_tusb();
    }
}


//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
    kbd_system.usb_hid_state = kbd_usb_hid_state_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    kbd_system.usb_hid_state = kbd_usb_hid_state_UNMOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    kbd_system.usb_hid_state = kbd_usb_hid_state_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    kbd_system.usb_hid_state = tud_mounted() ? kbd_usb_hid_state_MOUNTED : kbd_usb_hid_state_UNMOUNTED;
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
    (void)len;

    if(instance==ITF_NUM_HID1) {
        // for keyboard interface send next report
        uint8_t next_report_id = report[0] + 1;

        if (next_report_id < REPORT_ID_COUNT) {
            send_hid_report(next_report_id);
        }
    }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
// --not used here--
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize)
{
    if (report_type == HID_REPORT_TYPE_OUTPUT) {
        // Set keyboard LED e.g. Caps_Lock, Num_Lock etc..
        if (instance == ITF_NUM_HID1 && report_id == REPORT_ID_KEYBOARD) {
            // bufsize should be at least 1
            if (bufsize < 1)
                return;

            uint8_t const kbd_leds = buffer[0];
            hid_report_in_keyboard_t* keyboard = &kbd_system.core1.hid_report_in.keyboard;
            keyboard->NumLock = kbd_leds & KEYBOARD_LED_NUMLOCK;
            keyboard->CapsLock = kbd_leds & KEYBOARD_LED_CAPSLOCK;
            keyboard->ScrollLock = kbd_leds & KEYBOARD_LED_SCROLLLOCK;
            keyboard->Compose = kbd_leds & KEYBOARD_LED_COMPOSE;
            keyboard->Kana = kbd_leds & KEYBOARD_LED_KANA;
        } else if (instance == ITF_NUM_HID2) {
            // Process Generic In/Out
            switch(buffer[0]) {
            case 1:
                tud_hid_n_report(instance, 0, &tud_fail_counter, sizeof(tud_fail_counter));
                break;
            default: // 0 or any other
                tud_hid_n_report(instance, 0, &core0_debug, sizeof(core0_debug));
                break;
            }
        }
    }
}
