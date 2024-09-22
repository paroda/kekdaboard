#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico_fota_bootloader.h"

#include "data_model.h"
#include "hw_model.h"

#ifdef KBD_NODE_AP
#include "input_processor.h"
#include "usb_hid.h"
#endif

#ifdef KBD_NODE_LEFT

void rtc_task(void *param) {
  (void)param;
  rtc_read_time(kbd_hw.rtc, &kbd_system.core1.date);
  kbd_system.core1.temperature = rtc_get_temperature(kbd_hw.rtc);
}

void lcd_display_head_task(void *param) {
  (void)param;

  lcd_update_backlight(kbd_system.backlight);

  kbd_system_core1_t *c = &kbd_system.core1;
  static uint8_t old_state[11];
  uint8_t state[11] = {
      is_config_screen(kbd_system.screen),           // 0
      c->temperature,                                // 1
      c->date.hour,                                  // 2
      c->date.minute,                                // 3
      c->date.weekday % 8,                           // 4
      c->date.month,                                 // 5
      c->date.date,                                  // 6
      c->state.flags & KBD_FLAG_CAPS_LOCK ? 1 : 0,   // 7
      c->state.flags & KBD_FLAG_NUM_LOCK ? 1 : 0,    // 8
      c->state.flags & KBD_FLAG_SCROLL_LOCK ? 1 : 0, // 9
      kbd_system.wifi ? 1 : 0,                       // 10
  };

  if (memcmp(old_state, state, 11) == 0)
    return; // no change
  memcpy(old_state, state, 11);

  char txt[8];
  uint16_t bg = state[0] ? GRAY : BLACK;
  // uint16_t fgm = RED;
  uint16_t fg = WHITE;
  uint16_t fg1 = YELLOW;
  uint16_t bg2 = DARK_GRAY;
  uint16_t fgcl = 0xF800;
  uint16_t fgnl = 0x07E0;
  uint16_t fgsl = 0x001F;

  // 2 rows 240x(20+20)
  static lcd_canvas_t *cv = NULL;
  if (!cv)
    cv = lcd_new_canvas(240, 40, bg);
  lcd_canvas_rect(cv, 0, 0, 240, 40, bg, 1, true);

  sprintf(txt, "%s", state[10] ? "W" : "_");
  lcd_canvas_text(cv, 10, 8, txt, &lcd_font8, fg, bg);

  // row1: temperature, time, weekday
  // version : w:11x2=22 h:16, x:5-27
  // temperature: w:11x2=22 h:16, x:40-62
  sprintf(txt, "%02d", state[1]);
  lcd_canvas_text(cv, 40, 4, txt, &lcd_font16, fg, bg);
  // time (2 rows) : w:17x5=85 h:24, x:77-162, y:8
  sprintf(txt, "%02d:%02d", state[2], state[3]);
  lcd_canvas_text(cv, 80, 9, txt, &lcd_font24, fg1, bg);
  // weekday: w:11x3=33 h:16, x:197-230
  char *wd = rtc_week[state[4]];
  lcd_canvas_text(cv, 197, 4, wd, &lcd_font16, fg, bg);

  // MM/dd: w:11x5=55 h:16, x:180-235
  sprintf(txt, "%02d/%02d", state[5], state[6]);
  lcd_canvas_text(cv, 180, 22, txt, &lcd_font16, fg, bg);

  // row2: locksx3, MM/dd
  // locksx3: w:20x3=60 h:16, x:5-65
  lcd_canvas_rect(cv, 5, 22, 60, 16, bg2, 1, true);
  if (state[7]) // caps lock
    lcd_canvas_circle(cv, 15, 30, 5, fgcl, 1, true);
  if (state[8]) // num lock
    lcd_canvas_circle(cv, 35, 30, 5, fgnl, 1, true);
  if (state[9]) // scroll lock
    lcd_canvas_circle(cv, 55, 30, 5, fgsl, 1, true);

  lcd_display_canvas(kbd_hw.lcd, 0, 0, cv);
}

#endif

#ifdef KBD_NODE_RIGHT

static inline int16_t add_cap16_value(int16_t v1, int16_t v2) {
  int32_t v = v1;
  v += v2;
  // cap the value to 16 bits
  return (v > 0x7fff) ? 0x7fff : (-v > 0x7fff) ? -0x7fff : v;
}

void tb_scan_task_capture(void *param) {
  // sacn the track ball motion and accumulate
  kbd_tb_motion_t *ds = (kbd_tb_motion_t *)param;

  bool has_motion = false;
  bool on_surface = false;
  int16_t dx = 0;
  int16_t dy = 0;

  has_motion = tb_check_motion(kbd_hw.tb, &on_surface, &dx, &dy);

  ds->has_motion = ds->has_motion || has_motion;
  ds->on_surface = ds->on_surface || on_surface;
  ds->dx = add_cap16_value(ds->dx, dx);
  ds->dy = add_cap16_value(ds->dy, dy);
}

void tb_scan_task_publish(void *param) {
  // push out the accumulated motion deltas to shared buffer and clear out
  kbd_tb_motion_t *ds = (kbd_tb_motion_t *)param;

  write_shared_buffer(kbd_system.sb_tb_motion, time_us_64(), ds);

  ds->has_motion = false;
  ds->on_surface = false;
  ds->dx = 0;
  ds->dy = 0;
}

#endif

#ifdef KBD_NODE_AP

static bool reset_screen = true;

void process_idle() {
  static bool idle_prev = false;
  static uint8_t backlight_prev = 0;
  kbd_system_core1_t *c = &kbd_system.core1;
  // 0xFF means never idle
  uint8_t mins = c->idle_minutes == 0xFF ? 0 : (time_us_64() - c->active_ts) / 60000000u;
  bool idle = mins >= c->idle_minutes;
  if (idle & !idle_prev) { // turned idle
    backlight_prev = kbd_system.backlight;
    kbd_system.backlight = 0;
  } else if (idle_prev & !idle) { // turned active
    kbd_system.backlight = backlight_prev;
  }
  idle_prev = idle;
}

kbd_event_t process_basic_event(kbd_event_t event) {
  switch (event) {
  case kbd_backlight_event_HIGH:
    kbd_system.backlight = kbd_system.backlight <= 90 ? kbd_system.backlight + 10 : 100;
    return kbd_event_NONE;
  case kbd_backlight_event_LOW:
    kbd_system.backlight = kbd_system.backlight >= 10 ? kbd_system.backlight - 10 : 0;
    return kbd_event_NONE;
  case kbd_led_pixels_TOGGLE:
    kbd_system.pixels_on = !kbd_system.pixels_on;
    return kbd_event_NONE;
  default:
    return event;
  }
}

/*
 * Processing:
 * sb_left_key_press      -->  sb_state
 * sb_right_key_press          sb_left_task_request
 * sb_tb_motion                sb_right_task_request
 * sb_left_task_response       task_request
 * sb_right_task_response      hid_report_out
 * task_response
 * hid_report_in
 */

void process_inputs(void *param) {
  (void)param;

  kbd_system_core1_t *c = &kbd_system.core1;

  kbd_state_t *state = &c->state;
  kbd_state_t old_state = *state; // to check if changed

  // read the usb hid report received
  hid_report_in_t *hid_in = &c->hid_report_in;
  state->flags = (state->flags & (~KBD_FLAG_CAPS_LOCK) & (~KBD_FLAG_NUM_LOCK) & (~KBD_FLAG_SCROLL_LOCK)) |
                 (hid_in->keyboard.CapsLock ? KBD_FLAG_CAPS_LOCK : 0) |
                 (hid_in->keyboard.NumLock ? KBD_FLAG_NUM_LOCK : 0) |
                 (hid_in->keyboard.ScrollLock ? KBD_FLAG_SCROLL_LOCK : 0);

  // read the left/right task responses
  shared_buffer_t *sb = kbd_system.sb_left_task_response;
  if (sb->ts > c->left_task_response_ts)
    read_shared_buffer(sb, &c->left_task_response_ts, c->left_task_response);
  sb = kbd_system.sb_right_task_response;
  if (sb->ts > c->right_task_response_ts)
    read_shared_buffer(sb, &c->right_task_response_ts, c->right_task_response);

  // read the key_press
  uint64_t ts;
  read_shared_buffer(kbd_system.sb_left_key_press, &ts, c->left_key_press);
  read_shared_buffer(kbd_system.sb_right_key_press, &ts, c->right_key_press);

  // read tb_motion
  sb = kbd_system.sb_tb_motion;
  if (sb->ts > c->tb_motion_ts) {
    read_shared_buffer(sb, &c->tb_motion_ts, &c->tb_motion);
  } else {
    // discard the delta if already used
    c->tb_motion.dx = 0;
    c->tb_motion.dy = 0;
  }

  kbd_event_t event = kbd_event_NONE;
  if (reset_screen) {
    // switch to welcome screen after reset
    reset_screen = false;
    event = kbd_screen_event_INIT;
    c->active_ts = time_us_64();
  } else {
    // process inputs to update the hid_report_out and generate event
    event = execute_input_processor();

    // track activity
    hid_report_out_t *hid_out = &c->hid_report_out;
    if (event != kbd_event_NONE || hid_out->has_events)
      c->active_ts = time_us_64();
  }

  // handle basic event
  event = process_basic_event(event);

  // send event to screens processor to process the responses and raise requests if any
  handle_screen_event(event);

  // write the left/right task requests
  sb = kbd_system.sb_left_task_request;
  if (c->left_task_request_ts > sb->ts)
    write_shared_buffer(sb, c->left_task_request_ts, c->left_task_request);
  sb = kbd_system.sb_right_task_request;
  if (c->right_task_request_ts > sb->ts)
    write_shared_buffer(sb, c->right_task_request_ts, c->right_task_request);

  // note other state changes
  state->screen = kbd_system.screen;
  state->usb_hid_state = kbd_system.usb_hid_state;
  state->backlight = kbd_system.backlight;
  if (kbd_system.pixels_on)
    state->flags |= KBD_FLAG_PIXELS_ON;
  else
    state->flags &= (~KBD_FLAG_PIXELS_ON);

  // update the state if changed
  sb = kbd_system.sb_state;
  if (memcmp(&old_state, state, sizeof(kbd_state_t)) != 0) {
    c->state_ts = time_us_64();
    write_shared_buffer(sb, c->state_ts, state);
  }

  // transfer hid reports via usb
  usb_hid_task();
}

#else // LEFT/RIGHT

void pixel_task(void *param) {
  (void)param;

  if (!kbd_system.pixels_on) {
    led_pixel_set_off(kbd_hw.led_pixel);
    pixel_anim_reset(); // this can't be done by process_basic_event which is on AP
    return;
  }

  kbd_pixel_config_t *pc = &kbd_system.core1.pixel_config;
  uint32_t *colors = kbd_system.core1.pixel_colors;
  pixel_anim_update(colors, hw_led_pixel_count, pc->color, pc->anim_style, pc->anim_cycles);
  led_pixel_set(kbd_hw.led_pixel, colors);
}

void process_requests() {
  kbd_system_core1_t *c = &kbd_system.core1;
  if (kbd_system.sb_state->ts > c->state_ts) {
    read_shared_buffer(kbd_system.sb_state, &c->state_ts, &c->state);

    kbd_system.backlight = c->state.backlight;
    kbd_system.pixels_on = c->state.flags & KBD_FLAG_PIXELS_ON;
    kbd_system.usb_hid_state = c->state.usb_hid_state;
    kbd_system.screen = c->state.screen;
  }

  // get the request
  shared_buffer_t *sb = kbd_system.sb_task_request;
  if (sb->ts > kbd_system.core1.task_request_ts)
    read_shared_buffer(sb, &kbd_system.core1.task_request_ts, kbd_system.core1.task_request);
  // respond to request
  work_screen_task();
  // put the response
  sb = kbd_system.sb_task_response;
  if (kbd_system.core1.task_response_ts > sb->ts)
    write_shared_buffer(sb, kbd_system.core1.task_response_ts, kbd_system.core1.task_response);
}

#endif

void validate_comm_state(uint8_t index) {
  if (kbd_system.comm_state[index] == kbd_comm_state_reset) {
    kbd_system_core1_t *c = &kbd_system.core1;
    uint8_t buf[KBD_TASK_SIZE] = {0};

#ifdef KBD_NODE_AP
    // switch to welcome screen
    kbd_system.screen = kbd_info_screen_welcome;
    reset_screen = true;

    c->left_task_request_ts = 0;
    c->left_task_request[0] = 0;
    c->left_task_response_ts = 0;
    c->left_task_response[0] = 0;

    write_shared_buffer(kbd_system.sb_left_task_request, 0, buf);
    write_shared_buffer(kbd_system.sb_left_task_response, 0, buf);

    if (index == 0)
      memset(c->left_flash_data_pos, 0, KBD_CONFIG_SCREEN_COUNT);

    c->right_task_request_ts = 0;
    c->right_task_request[0] = 0;
    c->right_task_response_ts = 0;
    c->right_task_response[0] = 0;

    write_shared_buffer(kbd_system.sb_right_task_request, 0, buf);
    write_shared_buffer(kbd_system.sb_right_task_response, 0, buf);

    if (index == 1)
      memset(c->right_flash_data_pos, 0, KBD_CONFIG_SCREEN_COUNT);
#else
    c->task_request_ts = 0;
    c->task_request[0] = 0;
    c->task_response_ts = 0;
    c->task_response[0] = 0;

    write_shared_buffer(kbd_system.sb_task_request, 0, buf);
    write_shared_buffer(kbd_system.sb_task_response, 0, buf);
#endif

    // restore comm
    kbd_system.comm_state[index] = kbd_comm_state_init;
  }
}

////// MAIN

static void set_led(volatile kbd_led_state_t *led, kbd_led_state_t value) {
  if (*led != value)
    *led = value;
}

void core1_main() {
#ifdef KBD_NODE_AP
  kbd_system.led_left = kbd_led_state_BLINK_FAST;
  kbd_system.led_right = kbd_led_state_BLINK_FAST;
#else
  kbd_system.led = kbd_led_state_BLINK_FAST;
#endif

  uint32_t ts = board_millis();
#ifdef KBD_NODE_AP
  uint32_t proc_last_ms = ts;
#endif
#ifdef KBD_NODE_LEFT
  uint32_t lcd_last_ms = ts;
  uint32_t rtc_last_ms = ts;
#endif
#ifdef KBD_NODE_RIGHT
  uint32_t tb_capture_last_ms = ts;
  uint32_t tb_publish_last_ms = ts + 2;
#endif
#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
  uint32_t pixel_last_ms = ts;
#endif

#ifdef KBD_NODE_RIGHT

  kbd_tb_motion_t tbm = {.has_motion = false, .on_surface = false, .dx = 0, .dy = 0};

#endif

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
  kbd_system_core1_t *c = &kbd_system.core1;
#endif

  while (true) {

    if (kbd_system.firmware_downloading)
      return; // shutdown core1 on upgrade

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
    if (kbd_system.no_ap) {
      // typically the nodes are being just charged but not in operation
      // switch off lights and shutdown core1 on failed to connect to AP node
#ifdef KBD_NODE_LEFT
      lcd_update_backlight(0);
#endif
      kbd_hw.led_pixel->on = true;
      led_pixel_set_off(kbd_hw.led_pixel);
      return;
    }
#endif

    // apply any new configs
    apply_config_screen_data();

#ifdef KBD_NODE_LEFT

    // reset comm if needed
    validate_comm_state(0);

    // get date & and temperature, once a second
    do_if_elapsed(&rtc_last_ms, 1000, NULL, rtc_task);

    // update lcd display head, @ 100 ms
    do_if_elapsed(&lcd_last_ms, 100, NULL, lcd_display_head_task);

    // set the num lock led
    set_led(&kbd_system.led,
            kbd_system.ap_connected
                ? (kbd_system.usb_hid_state == kbd_usb_hid_state_MOUNTED
                       ? (c->state.flags & KBD_FLAG_NUM_LOCK ? kbd_led_state_ON : kbd_led_state_OFF)
                       : (kbd_system.usb_hid_state == kbd_usb_hid_state_UNMOUNTED ? kbd_led_state_BLINK_HIGH
                                                                                  : kbd_led_state_BLINK_LOW))
                : kbd_led_state_BLINK_FAST);

#endif

#ifdef KBD_NODE_RIGHT

    // reset comm if needed
    validate_comm_state(1);

    // scan track ball scroll (capture), @ 5 ms
    // scan at a high rate to eliminate trackball register overflow
    do_if_elapsed(&tb_capture_last_ms, 5, &tbm, tb_scan_task_capture);

    // publish track ball scroll (publish), @ 25 ms
    // publish at a lower rate than master process, to eliminate loss
    do_if_elapsed(&tb_publish_last_ms, 25, &tbm, tb_scan_task_publish);

    // set the caps lock led
    set_led(&kbd_system.led, kbd_system.ap_connected
                                 ? (c->state.flags & KBD_FLAG_CAPS_LOCK ? kbd_led_state_ON : kbd_led_state_OFF)
                                 : kbd_led_state_BLINK_FAST);

#endif

#ifdef KBD_NODE_AP

    // reset comm if needed
    validate_comm_state(0); // left comm
    validate_comm_state(1); // right comm

    // keep the tiny usb ready
    usb_hid_idle_task();

    // set board led to indicate USB
    switch (kbd_system.usb_hid_state) {
    case kbd_usb_hid_state_UNMOUNTED:
      set_led(&kbd_system.ledB, kbd_led_state_BLINK_FAST);
      break;
    case kbd_usb_hid_state_MOUNTED:
      set_led(&kbd_system.ledB, kbd_led_state_BLINK_NORMAL);
      break;
    case kbd_usb_hid_state_SUSPENDED:
      set_led(&kbd_system.ledB, kbd_led_state_BLINK_SLOW);
      break;
    }

    // process input to output/usb, @ 20 ms
    do_if_elapsed(&proc_last_ms, 20, NULL, process_inputs);

    // handle idelness
    process_idle();

    // check connection status
    kbd_system.left_active = board_millis() < (kbd_system.sb_left_key_press->ts / 1000) + 1000;
    kbd_system.right_active = board_millis() < (kbd_system.sb_right_key_press->ts / 1000) + 1000;

    // set left/right connection status led
    set_led(&kbd_system.led_left, kbd_system.left_active ? kbd_led_state_BLINK_NORMAL : kbd_led_state_BLINK_FAST);
    set_led(&kbd_system.led_right, kbd_system.right_active ? kbd_led_state_BLINK_NORMAL : kbd_led_state_BLINK_FAST);

#else // LEFT/RIGHT

    // set key switch leds, @ 50 ms
    do_if_elapsed(&pixel_last_ms, 50, NULL, pixel_task);

    // sync state and execute on-demand tasks
    process_requests();

    // check connection
    kbd_system.ap_connected = board_millis() < (c->state_ts / 1000) + 1000;

    // use board led for ap connection status
    set_led(&kbd_system.ledB, kbd_system.ap_connected ? kbd_led_state_BLINK_NORMAL : kbd_led_state_BLINK_LOW);

#endif

    sleep_ms(1);
  }
}
