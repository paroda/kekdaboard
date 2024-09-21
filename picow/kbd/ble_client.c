#include <stdint.h>
#include <string.h>

#include "btstack.h"
#include "pico/stdlib.h"

#include "ble_comm.h"

#define MAX_CLIENTS 2
uint8_t ble_client_ids[MAX_CLIENTS] = {BLE_COMM_LEFT_ID, BLE_COMM_RIGHT_ID};

typedef struct {
  gatt_client_notification_t listener;
  bd_addr_t addr;
  bd_addr_type_t addr_type;
  hci_con_handle_t con_handle;
  uint8_t *service_uuid;
  gatt_client_service_t service;
  uint8_t *rx_uuid;
  gatt_client_characteristic_t rx;
  uint8_t *tx_uuid;
  gatt_client_characteristic_t tx;
  btstack_context_callback_registration_t write;
  ble_comm_t comm;
} ble_client_t;

static ble_client_t ble_clients[MAX_CLIENTS];

static char *const ble_server_name = "kekdanode";

// addr and type of device with correct name

// On the GATT Server, RX Characteristic is used for receive data via Write, and
// TX Characteristic is used to send data via Notifications
static uint8_t ble_service_uuid[2][16] = {
    {0x00, 0x00, 0xFF, 0x10, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB},
    {0x00, 0x00, 0xFF, 0x20, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB}};
static uint8_t ble_characteristic_rx_uuid[2][16] = {
    {0x00, 0x00, 0xFF, 0x11, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB},
    {0x00, 0x00, 0xFF, 0x21, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB}};
static uint8_t ble_characteristic_tx_uuid[2][16] = {
    {0x00, 0x00, 0xFF, 0x12, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB},
    {0x00, 0x00, 0xFF, 0x22, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB}};

static btstack_packet_callback_registration_t hci_event_callback_registration;

static void handle_write(void *context);

static void init_client(ble_client_t *client) {
  client->con_handle = HCI_CON_HANDLE_INVALID;
  client->write.callback = handle_write;
  client->write.context = client;
  ble_comm_t *comm = &client->comm;
  comm->state = ble_state_IDLE;
}

static ble_client_t *find_client_by_addr(bd_addr_t addr) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    ble_client_t *client = ble_clients + i;
    for (int j = 0; j < BD_ADDR_LEN; j++)
      if (client->addr[j] != addr[j]) {
        client = NULL;
        break;
      }
    if (client)
      return client;
  }
  return NULL;
}

static ble_client_t *find_client_by_con_handle(hci_con_handle_t con_handle) {
  for (int i = 0; i < MAX_CLIENTS; i++)
    if (ble_clients[i].con_handle == con_handle)
      return ble_clients + i;
  return NULL;
}

static ble_client_t *find_client_by_id(uint8_t id) {
  for (int i = 0; i < MAX_CLIENTS; i++)
    if (ble_clients[i].comm.id == id)
      return ble_clients + i;
  return NULL;
}

ble_comm_t *ble_find_comm_by_id(uint8_t comm_id) {
  ble_client_t *client = find_client_by_id(comm_id);
  return client ? &client->comm : NULL;
}

// advertizer name should be same as the given name with a suffix letter, which is the id
static uint8_t identify_node(const char *name, uint8_t adv_len, const uint8_t *adv_data) {
  // get advertisement from report event
  uint16_t name_len = (uint8_t)strlen(name);

  // iterate over advertisement data
  ad_context_t context;
  for (ad_iterator_init(&context, adv_len, adv_data); ad_iterator_has_more(&context); ad_iterator_next(&context)) {
    uint8_t data_type = ad_iterator_get_data_type(&context);
    uint8_t data_size = ad_iterator_get_data_len(&context);
    const uint8_t *data = ad_iterator_get_data(&context);
    switch (data_type) {
    case BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME:
    case BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME:
      // compare prefix
      if (data_size != name_len + 1) // one more for suffix(id)
        break;
      if (memcmp(data, name, name_len) == 0)
        return data[name_len]; // return the suffix
      break;
    default:
      break;
    }
  }
  return 0;
}

void ble_initiate_send(uint8_t comm_id) {
  ble_client_t *client = find_client_by_id(comm_id);
  ble_comm_t *comm = &client->comm;
  comm->auto_send = true;
  gatt_client_request_to_write_without_response(&client->write, client->con_handle);
}

static void send_data(ble_client_t *client) {
  ble_comm_t *comm = &client->comm;
  if (client->con_handle == HCI_CON_HANDLE_INVALID || comm->state != ble_state_DATA)
    return;

  // send
  if (comm->send_len > 0) {
    gatt_client_write_value_of_characteristic_without_response(client->con_handle, client->rx.value_handle,
                                                               comm->send_len, comm->send_buff);
    comm->send_len = 0;
    gatt_client_request_to_write_without_response(&client->write, client->con_handle);
  } else {
    comm->auto_send = false;
  }
}

static void handle_write(void *context) {
  ble_client_t *client = (ble_client_t *)context;
  send_data(client);
}

static void gatt_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  UNUSED(packet_type);
  UNUSED(channel);
  UNUSED(size);

  uint16_t len;
  const uint8_t *buff;
  ble_client_t *client;
  uint8_t att_status;

  switch (hci_event_packet_get_type(packet)) {
  case GATT_EVENT_SERVICE_QUERY_RESULT:
    if ((client = find_client_by_con_handle(gatt_event_service_query_result_get_handle(packet))) &&
        client->comm.state == ble_state_SERVICE)
      gatt_event_service_query_result_get_service(packet, &client->service);
    break;
  case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
    if ((client = find_client_by_con_handle(gatt_event_characteristic_query_result_get_handle(packet))))
      switch (client->comm.state) {
      case ble_state_RX:
        gatt_event_characteristic_query_result_get_characteristic(packet, &client->rx);
        break;
      case ble_state_TX:
        gatt_event_characteristic_query_result_get_characteristic(packet, &client->tx);
        break;
      default:
        break;
      }
    break;
  case GATT_EVENT_QUERY_COMPLETE:
    if ((client = find_client_by_con_handle(gatt_event_query_complete_get_handle(packet)))) {
      att_status = gatt_event_query_complete_get_att_status(packet);
      if (att_status != ATT_ERROR_SUCCESS) {
        // printf("Query failed 0x%04x, ATT Error 0x%02x\n", client->con_handle, att_status);
        client->comm.state = ble_state_IDLE;
        gap_disconnect(client->con_handle);
        break;
      }
      switch (client->comm.state) {
      case ble_state_SERVICE:
        // service query complete, look for RX characteristic
        // printf("Get RX characteristic, 0x%04x\n", client->con_handle);
        client->comm.state = ble_state_RX;
        gatt_client_discover_characteristics_for_service_by_uuid128(gatt_event_handler, client->con_handle,
                                                                    &client->service, client->rx_uuid);
        break;
      case ble_state_RX:
        // RX query complete, look for TX characteristic
        // printf("Get TX characteristic, 0x%04x\n", client->con_handle);
        client->comm.state = ble_state_TX;
        gatt_client_discover_characteristics_for_service_by_uuid128(gatt_event_handler, client->con_handle,
                                                                    &client->service, client->tx_uuid);
        break;
      case ble_state_TX:
        // TX query complete, listen for notification
        // printf("Listen for notifictaion, 0x%04x\n", client->con_handle);
        gatt_client_listen_for_characteristic_value_updates(&client->listener, gatt_event_handler, client->con_handle,
                                                            &client->tx);
        gatt_client_write_client_characteristic_configuration(gatt_event_handler, client->con_handle, &client->tx,
                                                              GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
        client->comm.state = ble_state_NOTIFY;
        break;
      case ble_state_NOTIFY:
        // printf("Notifications enabled, 0x%04x\n", client->con_handle);
        ble_init_comm_data(&client->comm);
        client->comm.state = ble_state_DATA;
        gatt_client_request_to_write_without_response(&client->write, client->con_handle);
      case ble_state_DATA:
        break;
      default:
        break;
      }
    }
    break;
  case GATT_EVENT_NOTIFICATION:
    if ((client = find_client_by_con_handle(gatt_event_notification_get_handle(packet)))) {
      len = gatt_event_notification_get_value_length(packet);
      buff = gatt_event_notification_get_value(packet);
      /* printf("Got notification, 0x%04x, length: %u\n", client->con_handle, len); */
      ble_receive_data(&client->comm, buff, len);
    }
  default:
    break;
  }
}

static bool all_connected() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (ble_clients[i].con_handle == HCI_CON_HANDLE_INVALID)
      return false;
  }
  return true;
}

static void hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  UNUSED(channel);
  UNUSED(size);

  if (packet_type != HCI_EVENT_PACKET)
    return;

  static bool scanning = false;

  const uint8_t *adv_data;
  uint8_t adv_len;
  uint8_t id;
  bd_addr_t addr;
  hci_con_handle_t con_handle;
  ble_client_t *client;

  switch (hci_event_packet_get_type(packet)) {
  case BTSTACK_EVENT_STATE:
    // BTStack activated, get started
    if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
      // printf("Start scanning\n");
      gap_set_scan_parameters(0, 0x0030, 0x0030);
      gap_start_scan();
      scanning = true;
    }
    break;
  case GAP_EVENT_ADVERTISING_REPORT:
    // check name in advertisement
    adv_data = gap_event_advertising_report_get_data(packet);
    adv_len = gap_event_advertising_report_get_data_length(packet);
    if (!(id = identify_node(ble_server_name, adv_len, adv_data)))
      return;
    client = find_client_by_id(id);
    if (client->con_handle != HCI_CON_HANDLE_INVALID)
      return;
    // match connecting phy
    gap_set_connection_phys(1);
    // connect
    gap_event_advertising_report_get_address(packet, client->addr);
    client->addr_type = gap_event_advertising_report_get_address_type(packet);
    client->comm.state = ble_state_CONNECT;
    // printf("Connecting to node %u %s\n", client->id, bd_addr_to_str(client->addr));
    gap_connect(client->addr, client->addr_type);
    break;
  case HCI_EVENT_LE_META:
    // wait for connection complete
    switch (hci_event_le_meta_get_subevent_code(packet)) {
    case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
      hci_subevent_le_connection_complete_get_peer_address(packet, addr);
      if (!(client = find_client_by_addr(addr)))
        return;
      if (client->comm.state != ble_state_CONNECT)
        return;
      if (all_connected()) {
        gap_stop_scan();
        scanning = false;
      }
      client->comm.state = ble_state_SERVICE;
      client->con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
      // printf("Connected to node %s, 0x%04x searching service\n", bd_addr_to_str(client->addr), client->con_handle);
      gatt_client_discover_primary_services_by_uuid128(gatt_event_handler, client->con_handle, client->service_uuid);
      break;
    default:
      break;
    }
    break;
  case HCI_EVENT_DISCONNECTION_COMPLETE:
    // unregister listener
    con_handle = hci_event_disconnection_complete_get_connection_handle(packet);
    if (!(client = find_client_by_con_handle(con_handle)))
      return;
    if (client->comm.state == ble_state_NOTIFY || client->comm.state == ble_state_DATA) {
      gatt_client_stop_listening_for_characteristic_value_updates(&client->listener);
    }
    init_client(client);
    // printf("Disconnected %s, 0x%04x\n", bd_addr_to_str(client->addr), client->con_handle);
    if (!scanning) {
      gap_start_scan();
      scanning = true;
    }
    break;
  default:
    break;
  }
}

int ble_init() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    ble_client_t *client = ble_clients + i;
    init_client(client);
    client->comm.id = ble_client_ids[i];
    client->service_uuid = ble_service_uuid[i];
    client->rx_uuid = ble_characteristic_rx_uuid[i];
    client->tx_uuid = ble_characteristic_tx_uuid[i];
  }

  l2cap_init();

  sm_init();
  sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

  // sm_init needed before gatt_client_init
  gatt_client_init();

  hci_event_callback_registration.callback = &hci_event_handler;
  hci_add_event_handler(&hci_event_callback_registration);

  // turn on!
  hci_power_control(HCI_POWER_ON);

  return 0;
}
