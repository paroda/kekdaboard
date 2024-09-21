#ifndef _BLE_COMM_H
#define _BLE_COMM_

#include <stdbool.h>
#include <stdint.h>

#include "main.h"

#define BLE_COMM_LEFT_ID 'L'
#define BLE_COMM_RIGHT_ID 'R'

#define BLE_DATA_SIZE 64
#define BLE_QUEUE_SIZE 2

typedef enum {
  ble_state_IDLE,
  ble_state_CONNECT,
  ble_state_SERVICE,
  ble_state_RX,
  ble_state_TX,
  ble_state_NOTIFY,
  ble_state_DATA
} ble_state_t;

typedef struct {
  uint8_t id; // 1 to 255
  ble_state_t state;
  uint8_t send_buff[BLE_DATA_SIZE];
  uint8_t send_len;
  uint8_t recv_buff[BLE_QUEUE_SIZE][BLE_DATA_SIZE];
  uint8_t recv_len[BLE_QUEUE_SIZE];
  uint8_t recv_count;
  bool auto_send;
  // for sync
  uint8_t x;
  uint8_t y;
} ble_comm_t;

void ble_init_comm_data(ble_comm_t *comm);

ble_comm_t *ble_find_comm_by_id(uint8_t id);

int ble_init(void);

typedef void (*ble_consume_t)(uint8_t id, uint8_t *buff, uint8_t len);
typedef uint8_t (*ble_produce_t)(uint8_t id, uint8_t *buff);

void ble_process(uint8_t comm_id, ble_consume_t consume, ble_produce_t produce);

void ble_initiate_send(uint8_t comm_id);

void ble_receive_data(ble_comm_t *comm, const uint8_t *buff, uint16_t len);

#endif
