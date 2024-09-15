#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "btstack.h"
#include "pico/stdlib.h"

#include "ble_server.h" // generated from ble_server.gatt, see CMakeLists.txt

#define NODE_ID 'L'

const uint8_t adv_data[] = {
    // Flags general discoverable
    0x02,
    BLUETOOTH_DATA_TYPE_FLAGS,
    0x06,
    // Name
    0x0b,
    BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    'k',
    'e',
    'k',
    'd',
    'a',
    'n',
    'o',
    'd',
    'e',
    NODE_ID, // one extra letter for id
    // Incomplete List of 16-bit Service Class UUIDs -- FF10 - only valid for
    // testing!
    0x03,
    BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS,
    0x10,
    0xff,
};
const uint8_t adv_data_len = sizeof(adv_data);

static btstack_packet_callback_registration_t hci_event_callback_registration;

typedef enum { ble_state_IDLE, ble_state_DATA } ble_state_t;

#define DATA_SIZE 64
#define QUEUE_SIZE 2

// My state
typedef struct {
  ble_state_t state;
  hci_con_handle_t con_handle;
  uint16_t value_handle;
  uint8_t send_buff[DATA_SIZE];
  uint8_t send_len;
  uint8_t recv_buff[QUEUE_SIZE][DATA_SIZE];
  uint8_t recv_len[QUEUE_SIZE];
  uint8_t recv_count;
  bool auto_send;
  bool new_con;
} ble_server_t;

static ble_server_t server;

static void init_data() {
  server.recv_count = 0;
  memset(server.recv_len, 0, QUEUE_SIZE);
  server.send_len = 0;
  server.auto_send = true;
  server.new_con = true;
}

static void init_server() {
  server.state = ble_state_IDLE;
  server.con_handle = HCI_CON_HANDLE_INVALID;
}

static uint32_t counter = 0, drop_counter = 0;
static void receive_data(uint8_t *buff, uint16_t len) {
  counter++;
  if (server.recv_count < QUEUE_SIZE) {
    uint8_t pos = server.recv_count++;
    memcpy(server.recv_buff[pos], buff, len);
    server.recv_len[pos] = len;
  } else {
    drop_counter++;
  }
}

static void send_data() {
  if (server.con_handle == HCI_CON_HANDLE_INVALID || server.state != ble_state_DATA)
    return;

  // send
  if (server.send_len > 0) {
    att_server_notify(server.con_handle, server.value_handle, server.send_buff, server.send_len);
    server.send_len = 0;
    att_server_request_can_send_now_event(server.con_handle);
  } else {
    server.auto_send = false;
  }
}

static int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode,
                              uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
  UNUSED(offset);

  if (transaction_mode != ATT_TRANSACTION_MODE_NONE)
    return 0;
  switch (att_handle) {
  case ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE:
  case ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE:
    printf("client_config_handle: Written to 0x%04x, len %u\n", att_handle, buffer_size);
    bool notify = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    if (notify) {
      switch (att_handle) {
      case ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE:
        server.value_handle = ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE;
        break;
      case ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE:
        server.value_handle = ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE;
        break;
      default:
        break;
      }
      att_server_request_can_send_now_event(server.con_handle);
      server.state = ble_state_DATA;
      printf("Notification enabled\n");
    }
    break;
  case ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
  case ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
    /* printf("value_handle: Written to 0x%04x, len %u\n", att_handle, buffer_size); */
    receive_data(buffer, buffer_size);
    break;
  default:
    /* printf("default: Written to 0x%04x, len %u\n", att_handle, buffer_size); */
    break;
  }
  return 0;
}

static void att_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  UNUSED(channel);
  UNUSED(size);

  switch (packet_type) {
  case HCI_EVENT_PACKET:
    switch (hci_event_packet_get_type(packet)) {
    case ATT_EVENT_CONNECTED:
      // setup now
      if (server.con_handle != HCI_CON_HANDLE_INVALID)
        break; // already connected
      server.con_handle = att_event_connected_get_handle(packet);
      printf("ATT connected, handle 0x%04x\n", server.con_handle);
      gap_advertisements_enable(0);
      init_data();
      break;
    case ATT_EVENT_CAN_SEND_NOW:
      /* printf("ATT ready for send, handle 0x%04x\n", server.con_handle); */
      send_data();
      break;
    case ATT_EVENT_DISCONNECTED:
      if (server.con_handle != att_event_disconnected_get_handle(packet))
        break; // not my connection
      // free connection
      printf("ATT disconnected, handle 0x%04x\n", server.con_handle);
      init_server();
      gap_advertisements_enable(1);
      break;
    default:
      break;
    }
  default:
    break;
  }
}

static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  UNUSED(channel);
  UNUSED(size);

  if (packet_type != HCI_EVENT_PACKET)
    return;

  uint16_t con_interval;
  hci_con_handle_t con_handle;

  switch (hci_event_packet_get_type(packet)) {
  case HCI_EVENT_LE_META:
    switch (hci_event_le_meta_get_subevent_code(packet)) {
    case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
      con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
      con_interval = hci_subevent_le_connection_complete_get_conn_interval(packet);
      printf("- LE Connection 0x%04x: connected - interval %u.%02u ms, latency %u\n", con_handle,
             con_interval * 125 / 100, 25 * (con_interval & 3),
             hci_subevent_le_connection_complete_get_conn_interval(packet));
      printf("- LE Connection 0x%04x: request for interval change\n");
      // gap_request_connection_parameter_update(con_handle, 12, 12, 4, 0x0048); // 15 ms
      gap_request_connection_parameter_update(con_handle, 8, 8, 4, 0x0048); // 10 ms
      break;
    case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
      con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
      con_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
      printf("- LE Connection 0x%04x: connected - interval %u.%02u ms, latency %u\n", con_handle,
             con_interval * 125 / 100, 25 * (con_interval & 3),
             hci_subevent_le_connection_update_complete_get_conn_interval(packet));
      break;
    default:
      break;
    }
  default:
    break;
  }
}

int btstack_main(void) {
  init_server();

  l2cap_init();
  sm_init();

  // setup ATT server
  att_server_init(profile_data, NULL, att_write_callback);

  // register for HCI events
  hci_event_callback_registration.callback = &hci_packet_handler;
  hci_add_event_handler(&hci_event_callback_registration);

  // register for ATT events
  att_server_register_packet_handler(att_packet_handler);

  // setup advertisements
  bd_addr_t null_addr;
  memset(null_addr, 0, 6);
  gap_advertisements_set_params(0x30, 0x30, 0, 0, null_addr, 0x07, 0x00);
  gap_advertisements_set_data(adv_data_len, (uint8_t *)adv_data);
  gap_advertisements_enable(1);

  // turn on!
  hci_power_control(HCI_POWER_ON);

  return 0;
}

void btstack_process() {
  if (server.state != ble_state_DATA)
    return;

  static uint8_t y = 0, x = 0;

  if (server.new_con) {
    y = x = 0;
    server.new_con = false;
  }

  // consume received data
  if (server.recv_count > 0) {
    uint16_t yy = server.recv_buff[server.recv_count - 1][0];
    uint16_t yyy = y;
    yyy = yyy + server.recv_count;
    yyy = yyy > 255 ? yyy - 256 : yyy;
    if (yyy != yy)
      printf("Dropped somehing y %u, yy %u, recv_count %u\n", y, yy, server.recv_count);
    y = yy;
    server.recv_count = 0;
  }

  uint8_t xx = x == 255 ? 0 : x + 1;
  bool in_sync = x == y || xx == y;
  // if (!in_sync) printf("out of sync x %u, y %u\n", x, y);

  // produce data to send
  if (server.send_len == 0 && in_sync) {
    server.send_buff[0] = x = xx;
    server.send_len = DATA_SIZE;
    if (!server.auto_send) {
      server.auto_send = true;
      att_server_request_can_send_now_event(server.con_handle);
    }
  }

  // track
  static uint32_t ts = 0;
  if (ts == 0)
    ts = btstack_run_loop_get_time_ms();

  uint32_t t = btstack_run_loop_get_time_ms();
  if (t > ts + 5000) {
    uint32_t dt = t - ts;
    printf("recv %lu/s T %lu, drop %lu/s T %lu\n", counter * 1000 / dt, counter, drop_counter * 1000 / dt,
           drop_counter);
    counter = 0;
    drop_counter = 0;
    ts = t;
  }
}
