/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

#include "usb_descriptors.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "layouts.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

// blink interval in milliseconds to indicate device status
enum
{
  BLINK_NOT_MOUNTED = 100,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 3000,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

static void led_blinking_task(void);
static void hid_init(void);
static void hid_task(void);

static uint8_t kbd_modifiers = 0;
static uint8_t kbd_btns[6] = {0, 0, 0, 0, 0, 0};
static uint kbd_btns_count = 0;
static bool kbd_has_btns = false;
static bool kbd_had_keys = false;
static bool kbd_had_consumer_keys = false;

static bool kbd_num_lock_on = false;
static kbd_layer kbd_active_layer = KBD_LAYER_BASE;

/*------------- MAIN -------------*/
int main(void)
{
  board_init();
  tusb_init();
  hid_init();

  while (1)
  {
    tud_task(); // tinyusb device task
    led_blinking_task();
    hid_task();
  }

  return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

static void send_hid_kbd_report()
{
  if (kbd_has_btns)
  {
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, kbd_modifiers, kbd_btns);
    kbd_had_keys = true;
  }
  else
  {
    // send key report if previously has key pressed
    if (kbd_had_keys)
      tud_hid_keyboard_report(REPORT_ID_KEYBOARD, kbd_modifiers, kbd_btns);
    kbd_had_keys = false;
  }
}

static void send_hid_consumer_report()
{
  static bool mute_was_pressed = false;
  uint16_t consumer_key = 0;
  uint8_t btn = 0;

  if (kbd_has_btns)
  {
    for (uint i = 0; i < kbd_btns_count && !btn; i++)
    {
      btn = kbd_btns[i];
      switch (btn)
      {
      case HID_KEY_VOLUME_DOWN:
        consumer_key = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
        break;
      case HID_KEY_VOLUME_UP:
        consumer_key = HID_USAGE_CONSUMER_VOLUME_INCREMENT;
        break;
      case HID_KEY_MUTE:
        consumer_key = mute_was_pressed ? 0 : HID_USAGE_CONSUMER_MUTE;
        break;
      default:
        btn = 0;
        break;
      }
    }
  }

  mute_was_pressed = btn == HID_KEY_MUTE;

  if (consumer_key)
  {
    tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &consumer_key, 2);
    kbd_had_consumer_keys = true;
  }
  else
  {
    uint16_t empty_key = 0;
    if (kbd_had_consumer_keys)
      tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_key, 2);
    kbd_had_consumer_keys = false;
  }
}

static void send_hid_report(uint8_t report_id)
{
  if (!tud_hid_ready())
    return; //skip if hid is not ready yet

  // NOTE: you must send report for all previous reports in report id sequence
  //       otherwise your target report will never get called.
  //       For example, if you do not send the mouse report, then there would be
  //       no completion callback to send the next report which is consumer control.
  switch (report_id)
  {
  case REPORT_ID_KEYBOARD:
    send_hid_kbd_report();
    break;

  case REPORT_ID_MOUSE:
    // arguments: button = 0x00 - no button
    //            delta-right = 0, delta-down = 0
    //            scroll = 0, pan = 0
    tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, 0, 0, 0, 0);
    break;

  case REPORT_ID_CONSUMER_CONTROL:
    send_hid_consumer_report();
    break;

  case REPORT_ID_GAMEPAD:
    break;

  default:
    break;
  }
}

static void kbd_read_num_layer_btn(bool pressed)
{
  static bool was_pressed = false;

  if (!was_pressed && pressed)
  {
    kbd_active_layer = kbd_active_layer == KBD_LAYER_NUM ? KBD_LAYER_BASE : KBD_LAYER_NUM;
    bool num = kbd_active_layer == KBD_LAYER_NUM;
    gpio_put(kbd_led_num_layer_gpio, num);
    // also set the num lock on
    if (kbd_num_lock_on != num && kbd_btns_count<6) kbd_btns[kbd_btns_count++]=HID_KEY_NUM_LOCK;
  }

  was_pressed = pressed;
}

static void kbd_read_btn(const uint8_t key[2], bool pressed)
{
  uint8_t flag = key[0];
  uint8_t code = key[1];

  // read modifier flag
  if (pressed && flag)
  {
    kbd_modifiers |= flag;
  }

  // unassigned keys
  if (code == 0)
  {
    return;
  }

  // special key - num layer
  if (code == KBD_KEY_NUM_LAYER)
  {
    kbd_read_num_layer_btn(pressed);
    return;
  }

  // regular keys
  if (pressed && kbd_btns_count < 6)
  {
    kbd_btns[kbd_btns_count++] = code;
  }
}

static void kbd_buttons_read()
{
  for (uint row = 0; row < KBD_ROW_COUNT; row++)
  {
    uint row_gpio = kbd_layout_row_gpio[row];
    gpio_put(row_gpio, true);

    for (uint col = 0; col < KBD_COL_COUNT; col++)
    {
      uint col_gpio = kbd_layout_col_gpio[col];
      kbd_read_btn(kbd_layouts[kbd_active_layer][row][col], gpio_get(col_gpio));
    }

    gpio_put(row_gpio, false);
  }
}

// initialize GPIO pins
static void hid_init(void)
{

  uint i, gpio;

  // set led out pins - out
  gpio = kbd_led_caps_lock_gpio;
  gpio_init(gpio);
  gpio_set_dir(gpio, GPIO_OUT);

  gpio = kbd_led_num_lock_gpio;
  gpio_init(gpio);
  gpio_set_dir(gpio, GPIO_OUT);

  gpio = kbd_led_num_layer_gpio;
  gpio_init(gpio);
  gpio_set_dir(gpio, GPIO_OUT);

  // set row pins - out
  for (i = 0; i < KBD_ROW_COUNT; i++)
  {
    gpio = kbd_layout_row_gpio[i];
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
  }

  // set col pins - in
  for (i = 0; i < KBD_COL_COUNT; i++)
  {
    gpio = kbd_layout_col_gpio[i];
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
  }
}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
static void hid_task(void)
{
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if (board_millis() - start_ms < interval_ms)
    return; // not enough time
  start_ms += interval_ms;

  // clear
  kbd_modifiers = 0;
  for (uint i = 0; i < 6; i++)
    kbd_btns[i] = 0;
  kbd_btns_count = 0;

  // read
  kbd_buttons_read();

  // determine if has any buttons pressed
  kbd_has_btns = kbd_modifiers;
  for (uint i = 0; i < 6; i++)
  {
    kbd_has_btns |= kbd_btns[i];
  }

  // Remote wakeup
  if (tud_suspended() && kbd_has_btns)
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }
  else
  {
    // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
    send_hid_report(REPORT_ID_KEYBOARD);
  }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t itf, uint8_t const *report, uint8_t len)
{
  (void)itf;
  (void)len;

  uint8_t next_report_id = report[0] + 1;

  if (next_report_id < REPORT_ID_COUNT)
  {
    send_hid_report(next_report_id);
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void)itf;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
  (void)itf;

  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    // Set keyboard LED e.g. Caps_Lock, Num_Lock etc..
    if (report_id == REPORT_ID_KEYBOARD)
    {
      // bufsize should be at least 1
      if (bufsize < 1)
        return;

      uint8_t const kbd_leds = buffer[0];

      gpio_put(kbd_led_caps_lock_gpio, kbd_leds & KEYBOARD_LED_CAPSLOCK);

      kbd_num_lock_on = kbd_leds & KEYBOARD_LED_NUMLOCK;
      gpio_put(kbd_led_num_lock_gpio, kbd_num_lock_on);
    }
  }
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
static void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if (board_millis() - start_ms < blink_interval_ms)
    return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);

  led_state = 1 - led_state; // toggle
}
