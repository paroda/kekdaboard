#include "ble_comm.h"
#include <string.h>

void ble_init_comm_data(ble_comm_t *comm) {
  comm->recv_count = 0;
  memset(comm->recv_len, 0, BLE_QUEUE_SIZE);
  comm->send_len = 0;
  comm->auto_send = true;
  comm->x = 0;
  comm->y = 0;
}

void ble_process(uint8_t comm_id, ble_consume_t consume, ble_produce_t produce) {
  ble_comm_t *comm = ble_find_comm_by_id(comm_id);
  if (!comm || comm->state != ble_state_DATA)
    return;

  uint8_t *buff;
  uint8_t len;

  // consume received data
  while (comm->recv_count > 0) {
    buff = comm->recv_buff[comm->recv_count - 1];
    len = comm->recv_len[comm->recv_count - 1];
    comm->recv_count--;
    comm->y = buff[0];
    if (len > 1)
      consume(comm_id, buff + 1, len - 1);
  }

  uint8_t x = comm->x == 255 ? 0 : comm->x + 1;
  bool in_sync = comm->x == comm->y || x == comm->y;

  // produce data to send
  if (comm->send_len == 0 && in_sync) {
    buff = comm->send_buff;
    len = produce(comm_id, buff + 1);
    buff[0] = comm->x = x;
    comm->send_len = len + 1;
    if (!comm->auto_send)
      ble_initiate_send(comm_id);
  }
}

void ble_receive_data(ble_comm_t *comm, const uint8_t *buff, uint16_t len) {
  if (comm->recv_count < BLE_QUEUE_SIZE) {
    uint8_t pos = comm->recv_count++;
    memcpy(comm->recv_buff[pos], buff, len);
    comm->recv_len[pos] = len;
  }
}
