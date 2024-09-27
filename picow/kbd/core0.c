#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

#include "data_model.h"
#include "hw_model.h"

#include "ble_comm.h"
#include "util/shared_buffer.h"

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
#include "util/key_scan.h"
#endif

static void toggle_led(void *param) {
  kbd_led_t *led = (kbd_led_t *)param;
  led->on = !led->on;
  if (led->wl_led)
    cyw43_arch_gpio_put(led->gpio, led->on);
  else
    gpio_put(led->gpio, led->on);
}

static int32_t led_millis_to_toggle(kbd_led_t *led, kbd_led_state_t state) {
  // return milliseconds after which to toggle the led
  // return -1 to indicate skip toggle
  switch (state) {
  case kbd_led_state_OFF:
    return led->on ? 0 : -1;
  case kbd_led_state_ON:
    return led->on ? -1 : 0;
  case kbd_led_state_BLINK_LOW: // 100(on), 900(off)
    return led->on ? 100 : 900;
  case kbd_led_state_BLINK_HIGH: // 900, 100
    return led->on ? 900 : 100;
  case kbd_led_state_BLINK_SLOW: // 1000, 1000
    return led->on ? 2000 : 2000;
  case kbd_led_state_BLINK_NORMAL: // 500, 500
    return led->on ? 500 : 500;
  case kbd_led_state_BLINK_FAST: // 100, 100
    return led->on ? 50 : 50;
  }
  return -1;
}

static void led_task() {

#ifdef KBD_NODE_AP
  static uint32_t led_left_last_ms = 0;
  int32_t led_left_ms = led_millis_to_toggle(&kbd_hw.led_left, kbd_system.led_left);
  if (led_left_ms >= 0)
    do_if_elapsed(&led_left_last_ms, led_left_ms, &kbd_hw.led_left, toggle_led);

  static uint32_t led_right_last_ms = 0;
  int32_t led_right_ms = led_millis_to_toggle(&kbd_hw.led_right, kbd_system.led_right);
  if (led_right_ms >= 0)
    do_if_elapsed(&led_right_last_ms, led_right_ms, &kbd_hw.led_right, toggle_led);
#else
  static uint32_t led_last_ms = 0;
  int32_t led_ms = led_millis_to_toggle(&kbd_hw.led, kbd_system.led);
  if (led_ms >= 0)
    do_if_elapsed(&led_last_ms, led_ms, &kbd_hw.led, toggle_led);
#endif

  static uint32_t ledB_last_ms = 0;
  int32_t ledB_ms = led_millis_to_toggle(&kbd_hw.ledB, kbd_system.ledB);
  if (ledB_ms >= 0)
    do_if_elapsed(&ledB_last_ms, ledB_ms, &kbd_hw.ledB, toggle_led);
}

static void update_comm_state(volatile kbd_comm_state_t *comm_state, kbd_comm_state_t peer_state) {
  switch (*comm_state) {
  case kbd_comm_state_init:
    if (peer_state == kbd_comm_state_init || peer_state == kbd_comm_state_ready) {
      *comm_state = kbd_comm_state_ready;
    }
    break;
  case kbd_comm_state_ready:
    if (peer_state != kbd_comm_state_init) {
      *comm_state = kbd_comm_state_data;
    }
    break;
  case kbd_comm_state_data:
    if (peer_state == kbd_comm_state_init) {
      *comm_state = kbd_comm_state_reset;
    }
    break;
  default: // kbd_comm_state_reset
    // wait for core1 to update it to init
    break;
  }
}

typedef enum {
  comm_data_type_system_state = 0,
  comm_data_type_key_press,
  comm_data_type_tb_motion,
  comm_data_type_task_request,
  comm_data_type_task_response,
  comm_data_type_task_id,
} comm_data_type_t;

#ifdef KBD_NODE_AP

static uint8_t comm_ack_req_id[2] = {0, 0}; // last request acknowledged
static uint8_t comm_rcv_res_id[2] = {0, 0}; // last response received

static void comm_consume(uint8_t comm_id, uint8_t *buff, uint8_t len) {
  uint8_t index = comm_id == BLE_COMM_LEFT_ID ? 0 : 1;
  volatile kbd_comm_state_t *comm_state = kbd_system.comm_state + index;
  if (len < 1)
    return;
  update_comm_state(comm_state, (kbd_comm_state_t)buff[0]);
  len--;
  buff++;
  // check if ready for data
  if (*comm_state != kbd_comm_state_data)
    return;
  // read: key_press, tb_motion, task_response
  while (len > 0) {
    shared_buffer_t *sb = NULL;
    switch (buff[0]) {
    case comm_data_type_key_press:
      sb = index == 0 ? kbd_system.sb_left_key_press : kbd_system.sb_right_key_press;
      break;
    case comm_data_type_tb_motion:
      sb = kbd_system.sb_tb_motion;
      break;
    case comm_data_type_task_response:
      sb = index == 0 ? kbd_system.sb_left_task_response : kbd_system.sb_right_task_response;
      comm_rcv_res_id[index] = buff[1];
      break;
    case comm_data_type_task_id:
      comm_ack_req_id[index] = buff[1];
    default:
      break;
    }
    if (sb) {
      write_shared_buffer(sb, time_us_64(), buff + 1);
      len -= (1 + sb->size);
      buff += (1 + sb->size);
    } else if (buff[0] == comm_data_type_task_id) {
      len -= 2;
      buff += 2;
    } else {
      len = 0; // stop if encountered invalid
    }
  }
}

static uint8_t comm_produce(uint8_t comm_id, uint8_t *buff) {
  uint8_t index = comm_id == BLE_COMM_LEFT_ID ? 0 : 1;
  kbd_comm_state_t comm_state = kbd_system.comm_state[index];
  uint8_t len = 0;
  // while in reset mode, nothing to send
  if (comm_state == kbd_comm_state_reset)
    return 0;
  // pack the state as the first byte
  buff[0] = comm_state;
  len++;
  buff++;
  // append data if in data mode
  if (comm_state == kbd_comm_state_data) {
    // add system state
    static uint64_t state_ts = 0;
    buff[0] = comm_data_type_system_state;
    read_shared_buffer(kbd_system.sb_state, &state_ts, buff + 1);
    len += (1 + kbd_system.sb_state->size);
    buff += (1 + kbd_system.sb_state->size);
    // add request unless acknowledged
    static uint64_t req_ts[2] = {0, 0};
    shared_buffer_t *sb = index == 0 ? kbd_system.sb_left_task_request : kbd_system.sb_right_task_request;
    read_shared_buffer(sb, req_ts + index, buff + 1);
    if (buff[1] != comm_ack_req_id[index]) {
      buff[0] = comm_data_type_task_request;
      len += (1 + sb->size);
      buff += (1 + sb->size);
    }
    // acknowledge resposne received
    buff[0] = comm_data_type_task_id;
    buff[1] = comm_rcv_res_id[index];
    len += 2;
    buff += 2;
  }

  return len;
}

#else

static void key_scan_task() {
  // scan key presses with kbd_hw.ks and save to core0.key_press
  key_scan_update(kbd_hw.ks);
  memcpy(kbd_system.core0.key_press, kbd_hw.ks->keys, hw_row_count);
}

static uint8_t comm_rcv_req_id = 0; // last request received
static uint8_t comm_ack_res_id = 0; // last response acknowledged

static void comm_consume(uint8_t comm_id, uint8_t *buff, uint8_t len) {
  (void)comm_id;
  volatile kbd_comm_state_t *comm_state = kbd_system.comm_state;
  if (len < 1)
    return;
  update_comm_state(comm_state, (kbd_comm_state_t)buff[0]);
  len--;
  buff++;
  // check if ready for data
  if (*comm_state != kbd_comm_state_data)
    return;
  // read: system_state, task_request
  while (len > 0) {
    shared_buffer_t *sb = NULL;
    switch (buff[0]) {
    case comm_data_type_system_state:
      sb = kbd_system.sb_state;
      break;
    case comm_data_type_task_request:
      sb = kbd_system.sb_task_request;
      comm_rcv_req_id = buff[1];
      break;
    case comm_data_type_task_id:
      comm_ack_res_id = buff[1];
    default:
      break;
    }
    if (sb) {
      write_shared_buffer(sb, time_us_64(), buff + 1);
      len -= (1 + sb->size);
      buff += (1 + sb->size);
    } else if (buff[0] == comm_data_type_task_id) {
      len -= 2;
      buff += 2;
    } else {
      len = 0; // stop if encountered invalid
    }
  }
}

static uint8_t comm_produce(uint8_t comm_id, uint8_t *buff) {
  (void)comm_id;
  kbd_comm_state_t comm_state = *kbd_system.comm_state;
  uint8_t len = 0;
  // while in reset mode, nothing to send
  if (comm_state == kbd_comm_state_reset)
    return 0;
  // pack the state as the first byte
  buff[0] = comm_state;
  len++;
  buff++;
  // append data if in data mode
  if (comm_state == kbd_comm_state_data) {
    // add key_press
    buff[0] = comm_data_type_key_press;
    memcpy(buff + 1, kbd_system.core0.key_press, hw_row_count);
    len += (1 + hw_row_count);
    buff += (1 + hw_row_count);
#ifdef KBD_NODE_RIGHT
    // add tb_motion
    static uint64_t tbm_ts = 0;
    if (kbd_system.sb_tb_motion->ts > tbm_ts) {
      buff[0] = comm_data_type_tb_motion;
      read_shared_buffer(kbd_system.sb_tb_motion, &tbm_ts, buff + 1);
      len += (1 + kbd_system.sb_tb_motion->size);
      buff += (1 + kbd_system.sb_tb_motion->size);
    }
#endif
    // add response unless acknowledged
    static uint64_t res_ts = 0;
    read_shared_buffer(kbd_system.sb_task_response, &res_ts, buff + 1);
    if (buff[1] != comm_ack_res_id) {
      buff[0] = comm_data_type_task_response;
      len += (1 + kbd_system.sb_task_response->size);
      buff += (1 + kbd_system.sb_task_response->size);
    }
    // acknowledge request received
    buff[0] = comm_data_type_task_id;
    buff[1] = comm_rcv_req_id;
    len += 2;
    buff += 2;
  }

  return len;
}

static void check_if_wifi_requested() {
  // just see if any key is pressed at startup
  key_scan_update(kbd_hw.ks);
  for (int i = 0; i < hw_row_count; i++)
    if (kbd_hw.ks->keys[i] > 0) {
      kbd_system.wifi = true;
      return;
    }
}

static bool is_ap_long_lost() {
  static uint32_t ts = 0;
  if (kbd_system.ap_connected)
    ts = board_millis();
  if ((board_millis() - ts) > 30000)
    return true;
  return false;
}

static void stop_system() {
  kbd_system.no_ap = true;
  cyw43_arch_disable_sta_mode();
  hci_power_control(HCI_POWER_OFF);
  cyw43_arch_deinit();
  gpio_put(kbd_hw.led.gpio, 0);
  while (true)
    sleep_ms(60000);
}

#endif // <- end of #ifdef KBD_NODE_AP

static void wifi_task() {
  cyw43_arch_poll(); // do wifi tasks

  // disable wifi after 10 minutes, which should enough for downloading firmware
  if (board_millis() > 600000) {
#ifdef KBD_NODE_AP
    cyw43_arch_disable_ap_mode();
#else
    cyw43_arch_disable_sta_mode();
#endif
  }
}

static void bt_poll_handler(struct btstack_timer_source *timer) {
  if (kbd_system.firmware_downloading) {
    btstack_run_loop_set_timer(timer, 1); // wifi is primary
  } else {
    led_task();

#ifdef KBD_NODE_AP
    ble_process(BLE_COMM_LEFT_ID, comm_consume, comm_produce);
    ble_process(BLE_COMM_RIGHT_ID, comm_consume, comm_produce);

    btstack_run_loop_set_timer(timer, 5); // repeat after 5 ms
    btstack_run_loop_add_timer(timer);
#else
    key_scan_task();
    ble_process(0, comm_consume, comm_produce); // comm_id ignored

    if (is_ap_long_lost()) {
      btstack_run_loop_trigger_exit();
    } else {
      btstack_run_loop_set_timer(timer, 5); // repeat after 5 ms
      btstack_run_loop_add_timer(timer);
    }
#endif
  }

  if (kbd_system.wifi)
    wifi_task();
}

static btstack_timer_source_t bt_poll;

void core0_main() {
#ifdef KBD_NODE_AP
  kbd_system.wifi = true;
  cyw43_arch_enable_ap_mode(hw_ap_name, hw_ap_password, CYW43_AUTH_WPA2_AES_PSK);

  tcp_server_open(&kbd_system.core0.tcp_server, KBD_NODE_NAME);
#else
  check_if_wifi_requested();

  if (kbd_system.wifi) {
    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(hw_ap_name, hw_ap_password, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
      // missing AP, probably just chargin. just sleep until manually rebooted
      stop_system();
    }

    tcp_server_open(&kbd_system.core0.tcp_server, KBD_NODE_NAME);
  }
#endif

  ble_init();

  bt_poll.process = &bt_poll_handler;
  btstack_run_loop_set_timer(&bt_poll, 1);
  btstack_run_loop_add_timer(&bt_poll);

  btstack_run_loop_execute();

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
  stop_system();
#endif
}
