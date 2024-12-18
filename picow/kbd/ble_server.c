#include <stdint.h>
#include <string.h>

#include "btstack.h"
#include "pico/stdlib.h"

#include "ble_comm.h"

#ifdef KBD_NODE_LEFT
#include "ble_server_left.h" // generated from ble_server.gatt, see CMakeLists.txt
#define COMM_ID BLE_COMM_LEFT_ID
#define SVC 0x10
#define ATT_RX_CCH ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE
#define ATT_RX_VH ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
#define ATT_TX_CCH ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE
#define ATT_TX_VH ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
#endif

#ifdef KBD_NODE_RIGHT
#include "ble_server_right.h" // generated from ble_server.gatt, see CMakeLists.txt
#define COMM_ID BLE_COMM_RIGHT_ID
#define SVC 0x20
#define ATT_RX_CCH ATT_CHARACTERISTIC_0000FF21_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE
#define ATT_RX_VH ATT_CHARACTERISTIC_0000FF21_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
#define ATT_TX_CCH ATT_CHARACTERISTIC_0000FF22_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE
#define ATT_TX_VH ATT_CHARACTERISTIC_0000FF22_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
#endif

// My state
typedef struct {
  hci_con_handle_t con_handle;
  uint16_t value_handle;
  ble_comm_t comm;
} ble_server_t;

static ble_server_t ble_server;

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
    // one extra letter for id
    COMM_ID,
    // Incomplete List of 16-bit Service Class UUIDs -- FF10 - only valid for
    // testing!
    0x03,
    BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS,
    SVC,
    0xff,
};
const uint8_t adv_data_len = sizeof(adv_data);

static btstack_packet_callback_registration_t hci_event_callback_registration;

static void init_server(ble_server_t *server) {
  server->con_handle = HCI_CON_HANDLE_INVALID;
  ble_comm_t *comm = &ble_server.comm;
  comm->state = ble_state_IDLE;
}

ble_comm_t *ble_find_comm_by_id(uint8_t comm_id) {
  (void)comm_id;
  return &ble_server.comm;
}

void ble_initiate_send(uint8_t comm_id) {
  ble_server_t *server = &ble_server;
  ble_comm_t *comm = &ble_server.comm;
  comm->auto_send = true;
  att_server_request_can_send_now_event(server->con_handle);
}

static void send_data(ble_server_t *server) {
  ble_comm_t *comm = &ble_server.comm;
  if (server->con_handle == HCI_CON_HANDLE_INVALID || comm->state != ble_state_DATA)
    return;

  // send
  if (comm->send_len > 0) {
    att_server_notify(server->con_handle, server->value_handle, comm->send_buff, comm->send_len);
    comm->send_len = 0;
    att_server_request_can_send_now_event(server->con_handle);
  } else {
    comm->auto_send = false;
  }
}

static int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode,
                              uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
  UNUSED(offset);
  ble_server_t *server = &ble_server;
  ble_comm_t *comm = &ble_server.comm;
  if (transaction_mode != ATT_TRANSACTION_MODE_NONE)
    return 0;
  switch (att_handle) {
  case ATT_RX_CCH:
  case ATT_TX_CCH:
    bool notify = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    if (notify) {
      switch (att_handle) {
      case ATT_RX_CCH:
        server->value_handle = ATT_RX_VH;
        break;
      case ATT_TX_CCH:
        server->value_handle = ATT_TX_VH;
        break;
      default:
        break;
      }
      att_server_request_can_send_now_event(server->con_handle);
      comm->state = ble_state_DATA;
    }
    break;
  case ATT_RX_VH:
  case ATT_TX_VH:
    ble_receive_data(comm, buffer, buffer_size);
    break;
  default:
    break;
  }
  return 0;
}

static void att_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  UNUSED(channel);
  UNUSED(size);
  ble_server_t *server = &ble_server;

  switch (packet_type) {
  case HCI_EVENT_PACKET:
    switch (hci_event_packet_get_type(packet)) {
    case ATT_EVENT_CONNECTED:
      // setup now
      if (server->con_handle != HCI_CON_HANDLE_INVALID)
        break; // already connected
      server->con_handle = att_event_connected_get_handle(packet);
      gap_advertisements_enable(0);
      ble_init_comm_data(&server->comm);
      break;
    case ATT_EVENT_CAN_SEND_NOW:
      send_data(server);
      break;
    case ATT_EVENT_DISCONNECTED:
      if (server->con_handle != att_event_disconnected_get_handle(packet))
        break; // not my connection
      // free connection
      init_server(&ble_server);
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

  // uint16_t con_interval;
  hci_con_handle_t con_handle;

  switch (hci_event_packet_get_type(packet)) {
  case HCI_EVENT_LE_META:
    switch (hci_event_le_meta_get_subevent_code(packet)) {
    case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
      con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
      // con_interval = hci_subevent_le_connection_complete_get_conn_interval(packet);
      // printf("- LE Connection 0x%04x: connected - interval %u.%02u ms, latency %u\n", con_handle,
      //        con_interval * 125 / 100, 25 * (con_interval & 3),
      //        hci_subevent_le_connection_complete_get_conn_interval(packet));
      // printf("- LE Connection 0x%04x: request for interval change\n");
      // gap_request_connection_parameter_update(con_handle, 12, 12, 4, 0x0048); // 15 ms
      gap_request_connection_parameter_update(con_handle, 8, 8, 4, 0x0048); // 10 ms
      break;
    case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
      con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
      // con_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
      // printf("- LE Connection 0x%04x: connected - interval %u.%02u ms, latency %u\n", con_handle,
      //        con_interval * 125 / 100, 25 * (con_interval & 3),
      //        hci_subevent_le_connection_update_complete_get_conn_interval(packet));
      break;
    default:
      break;
    }
  default:
    break;
  }
}

int ble_init(void) {
  init_server(&ble_server);
  ble_server.comm.id = COMM_ID;

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
