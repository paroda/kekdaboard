#ifndef _PEER_COMM_H
#define _PEER_COMM_H

#include "shared_buffer.h"

/*
 * Peer communication
 *
 * Out of the two units (left/right), only one would connect to the USB host
 * and would interact with the other unit via UART serial communication.
 * Due to current hardware limitations,the communication would be always one byte
 * at a time.
 *
 * SENDING DATA
 *
 * The first bit of the byte is reserved to classify it as either command byte(1) or data byte(0).
 * Hence, one byte can carry  only 7bits of data. For the 8th bit (MSB), they are collected
 * and send later as another byte (0b10xxxxxx). The first two bits (10) denotes that it is
 * the SET_MSB command carrying MSB bits of last six bytes sent earlier. However, we don't need
 * to send the SET_MSB, if none of the last six bytes had the MSB bit, and thus minimize load.
 *
 * This is a limited communication mechanism. Each unit has a fixed set of buffers with an id.
 * The other unit will send the data for these predefined buffers only.
 *
 * The transmission starts with the sender sending command byte INIT_DATA(0b110xxxxx). The
 * last five bits identifies the data being transfered `xxxxx`. Thus it can handle upto 32 data
 * objects.
 *
 * The receiver would then send its own byte. It doesn't matter what this byte is, as far as
 * this transmission operation is concerned. The sender only needs that byte as a confirmation
 * to send the next byte, which would be the actual data byte. And so it continues till there is
 * no more data bytes to send. The receiver already knows where to put these bytes, from the
 * id number (0-31) from the command byte (INIT_DATA+DATA_ID) that initiated this transfer.
 *
 * Now, since the return bytes are free to be anything, it can actually be used by the receiver
 * to send its own data. So it can send its own command byte and then send the data bytes, on
 * its turn to send the return bytes. if there is nothing to send then it must send the empty
 * command byte NO_DATA(0xF0).
 *
 * The transmission is done, when a unit receives the NO_DATA, and has no data to send.
 *
 * FLOW CONTROL
 *
 * The pico core-1 will be solely responsible for actual TX/RX. Periodically, it would
 * send the command byte INIT_CYCLE(0xF3) to start one transfer cycle. When the
 * the transfer is complete, the last unit to receive the byte NO_DATA would send the byte
 * END_CYCLE(0xF4) to let the other unit know as well that the cycle is finished.
 * The pico core-0 would set the data to transmit and core-1 would send them on its turn.
 *
 * PEER SETUP
 *
 * The unit that successfully establishes the USB host connection, would be the master unit.
 * It would then send out a PING(0xF1).
 *
 * The other unit would stop any further attempt with USB, and send back a PEER_READY(0xF2).
 * After that, both units know the other to be ready, and can do there periodic transmission.
 * The transmission would always be initiated by the master unit at a fixed interval, and then
 * both unit will continue sending their bytes one by one, till they have sent all they have.
 */

#define peer_comm_BUFF_SIZE 256 // the maximum size of temp buffer to hold data being transmitted

typedef struct {
    uint8_t size; // number of predefined datasets
    shared_buffer_t** datasets;

    uint8_t* data_inits; // 0 for in dataset, but for out dataset INIT_DATA+DATA_ID
    uint64_t* data_ts;   // dataset, to check if out dataset has new data to send

    volatile bool busy; // indicates if currently engaged in transmission

    uint8_t in_buff[peer_comm_BUFF_SIZE];
    uint8_t in_size;  // <= 256
    uint8_t in_id;    // track current incoming dataset, < 32
    uint8_t in_pos;   // track position in incoming buffer, < 256
    bool in_active;   // if in the middle of receiving an incoming dataset

    uint8_t out_buff[peer_comm_BUFF_SIZE];
    uint8_t out_size;  // <= 256
    uint8_t out_id;    // track current outgoing dataset, < 32
    uint8_t out_pos;   // track position in outgoing buffer, < 256
    uint8_t out_stage; // track stage of outgoing transfer

    uint8_t msbs;      // track the msb bits previously sent data bytes (at most six)

    volatile uint32_t last_state; // used for detecting if transfer has hanged

    volatile bool peer_ready; // indicates comm established with the other unit

    uint8_t (*get) (void); // function to get the incoming byte
    void (*put) (uint8_t); // function to put the outgoing byte
    uint64_t (*current_ts) (void); // function to get the current time in us
} peer_comm_config_t;

peer_comm_config_t* new_peer_comm_config(uint8_t size, shared_buffer_t** datasets, uint8_t* data_inits);

void peer_comm_set_handlers(peer_comm_config_t* pcc,
                            uint8_t (*get) (void), void (*put) (uint8_t), uint64_t (*current_ts) (void));

void free_peer_comm_config(peer_comm_config_t* pcc);

uint8_t peer_comm_cmd_init_data(uint8_t data_id);
/*
 * To be called by the master unit, periodically, to start one transfer cycle
 */
void peer_comm_init_cycle(peer_comm_config_t* pcc);

void peer_comm_ping(peer_comm_config_t* pcc);

void peer_comm_on_receive(peer_comm_config_t* pcc);

#endif
