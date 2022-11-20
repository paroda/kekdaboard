#ifndef _PEER_COMM_H
#define _PEER_COMM_H

#include "shared_buffer.h"

#ifdef __cplusplus
extern "C"
{
#endif

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
     * The first bit of the byte is reserved to classify it as either command byte(1) or data byte(0).a
     * This is a limited communication mechanism. Only a predefined set of data understood by both
     * both units are exchanged.
     *
     * The transmission starts with the sender sending command byte INIT_DATA(0b1001xxxx). The
     * last four bits identifies the data being transfered `xxxx`. Thus it can handle upto 16 data
     * objects.
     *
     * The receiver would then send its own byte. It doesn't matter what this byte is, as far as
     * this transmission operation is concerned. The sender only needs that byte as a confirmation
     * to send the next byte, which would be the actual data byte. And so it continues till there is
     * no more data bytes to send. The receiver already knows where to put these bytes, from the
     * id number (0-15) from the command byte that initiated this transfer.
     *
     * Now, since the return bytes are free to be anything, it can actually be used by the receiver
     * to send its own data. So it can send its own command byte and then send the data bytes, on
     * its turn to send the return bytes. if there is nothing to send then it must send the empty
     * command byte COMMAND(0b10000000).
     *
     * The end of transmission is achieved, when a unit receives the empty command byte (COMMAND)
     * but has no data to send.
     *
     * FLOW CONTROL
     *
     * The pico core-1 will be solely responsible for actual TX/RX. Periodically, it would
     * send the command byte INIT_CYCLE(0b11000000) to start one transfer cycle. When the
     * the transfer is complete, the last unit to receive the byte COMMAND would send the byte
     * DONE_CYCLE(0b11010000) to let the other unit know as well that the cycle is finished.
     * The pico core-0 would set the data to transmit and core-1 would send them on its turn.
     *
     * PEER SETUP
     *
     * The unit that successfully establishes the USB host connection, would send out a PING(0b10010000).
     * The other unit would stop any further attempt with USB, and send back a READY(0b10100000).
     * After that, both units know the other to be ready, and can do there periodic transmission.
     * The transmission would always be initiated by the first unit at a fixed interval, and then
     * both unit will continue sending their bytes one by one, till they have sent all they have.
     */

#define peer_comm_byte_COMMAND       0b10000000
#define peer_comm_byte_INIT_DATA     0b10010000 // last four bit to replaced with data ID
#define peer_comm_byte_PING          0b10100000
#define peer_comm_byte_READY         0b10110000
#define peer_comm_byte_INIT_CYCLE    0b11000000 // sent by primary unit to initiate one transfer cycle
#define peer_comm_byte_DONE_CYCLE    0b11010000 // sent by primary unit to initiate one transfer cycle
#define peer_comm_byte_DATA_ID_MASK  0b00001111
#define peer_comm_byte_COMMAND_MASK  0b11110000
#define peer_comm_byte_DATA_MASK     0b01111111  // only 7bits used for data

#define peer_comm_DATA_ID_NONE 0xf0 // indicates initial state, to start with first out dataset
#define peer_comm_DATA_ID_DONE 0xff // indicates finished state, no more datasets to send

    typedef struct {
        uint8_t size; // number of predefined datasets (an array of bytes)
        shared_buffer_t** datasets;
        uint8_t* data_inits; // 0 for incoming dataset, but for outgoing dataset INIT_DATA+DATA_ID

        bool busy; // indicates if currently engaged in transmission
        uint8_t current_in_id;  // track current incoming dataset, < 16
        uint8_t current_in_pos;
        uint8_t current_out_id; // track current outgoing dataset, < 16
        uint8_t current_out_pos;
        uint32_t last_state; // used for detecting if transfer has hanged

        bool peer_ready; // indicates comm established with the other unit

        uint8_t (*get) (void); // function to get the incoming byte
        void (*put) (uint8_t); // function to put the outgoing byte
        uint64_t (*current_ts) (void); // function to get the current time in us
    } peer_comm_config_t;

    void peer_comm_reset_state(peer_comm_config_t* pcc) {
        pcc->busy = false;
        pcc->current_in_id = peer_comm_DATA_ID_NONE; // something >= 16 to indicate none
        pcc->current_in_pos = 0;
        pcc->current_out_id = peer_comm_DATA_ID_NONE; // something >= 16 to indicate none
        pcc->current_out_pos = 0;
        pcc->last_state = 0;
    }

    peer_comm_config_t* new_peer_comm_config(uint8_t size,
                                             shared_buffer_t** datasets,
                                             uint8_t* data_inits,
                                             uint8_t (*get) (void),
                                             void (*put) (uint8_t),
                                             uint64_t (*current_ts) (void)) {
        peer_comm_config_t* pcc = (peer_comm_config_t*) malloc(sizeof(peer_comm_config_t));

        pcc->size = size;
        pcc->datasets = (shared_buffer_t**) malloc(sizeof(shared_buffer_t*) * size);
        pcc->data_inits = (uint8_t*) malloc(sizeof(uint8_t) * size);
        for(int i=0; i<size; i++) {
            pcc->datasets[i] = datasets[i];
            pcc->data_inits[i] = data_inits[i];
        }

        pcc->peer_ready = false;
        peer_comm_reset_state(pcc);

        pcc->get = get;
        pcc->put = put;
        pcc->current_ts = current_ts;
        return pcc;
    }

    void free_peer_comm_config(peer_comm_config_t* pcc) {
        free(pcc->data_inits);
        free(pcc->datasets);
        free(pcc);
    }

    /*
     * To be called by the unit connected to USB host, periodically, to start one transfer cycle
     */
    void peer_comm_init_transfer(peer_comm_config_t* pcc) {
        if(pcc->busy) {
            uint32_t current_state;
            uint8_t* p = (uint8_t*) &current_state;
            p[0] = pcc->current_in_id;
            p[1] = pcc->current_in_pos;
            p[2] = pcc->current_out_id;
            p[2] = pcc->current_out_pos;
            // no need to intervene if state has changed indicating no hanged state
            if(pcc->last_state!=current_state) {
                pcc->last_state = current_state;
                return; // skip this cycle
            }
        }
        // set busy and start the transfer cycle
        peer_comm_reset_state(pcc);
        pcc->busy = true;
        pcc->put(peer_comm_byte_INIT_CYCLE);
    }

    void peer_comm_ping(peer_comm_config_t* pcc) {
        pcc->put(peer_comm_byte_PING);
    }

    /*
     * Either send the INIT_DATA for next dataset out
     * or data byte for next pos of current dataset out
     * Clears busy flag when done and informs the other unit
     */
    void peer_comm_emit_next_byte(peer_comm_config_t* pcc, bool peer_done) {
        uint8_t id = pcc->current_out_id;
        shared_buffer_t* sb = id<pcc->size ? pcc->datasets[id] : NULL;

        if(sb && pcc->current_in_pos < sb->size) {
            pcc->put(sb->buff_out[pcc->current_out_pos++]);
            return;
        }

        if(id != peer_comm_DATA_ID_DONE) {
            for(id = (id==peer_comm_DATA_ID_NONE ? 0 : id+1); id < pcc->size; id++) {
                if(pcc->data_inits[id]>0
                   && update_shared_buffer_to_read(pcc->datasets[id])) {
                    pcc->current_out_id = id;
                    pcc->current_out_pos = 0;
                    pcc->put(pcc->data_inits[id]);
                    return;
                }
            }
            pcc->current_out_id = peer_comm_DATA_ID_DONE;
        }

        if(peer_done) {
            pcc->put(peer_comm_byte_DONE_CYCLE);
            pcc->busy = false;
        } else {
            pcc->put(peer_comm_byte_COMMAND);
        }
    }

    /*
     * save the next byte and mark dataset finished when last pos is reached
     */
    void peer_comm_save_next_data_byte(peer_comm_config_t* pcc, uint8_t d) {
        uint8_t id = pcc->current_in_id;
        shared_buffer_t* sb = id<pcc->size ? pcc->datasets[id] : NULL;

        if(sb && pcc->current_in_pos < sb->size) {
            sb->buff_in[pcc->current_in_pos++] = d;
            if(pcc->current_in_pos >= sb->size) {
                sb->ts_in_end = sb->ts_in_start;
                pcc->current_in_id = peer_comm_DATA_ID_NONE;
            }
        }
    }

    /*
     * set the current dataset to receive the next incoming bytes
     */
    void peer_comm_set_incoming_dataset(peer_comm_config_t* pcc, uint8_t data_id) {
        pcc->current_in_id = data_id;
        pcc->current_in_pos = 0;
        pcc->datasets[data_id]->ts_in_start = pcc->current_ts();
    }

    void peer_comm_on_receive(peer_comm_config_t* pcc) {
        uint8_t b = pcc->get(); // get the received byte
        if(b & peer_comm_byte_COMMAND) { // received a command byte
            switch(b & peer_comm_byte_COMMAND_MASK) {
            case peer_comm_byte_COMMAND:
                peer_comm_emit_next_byte(pcc, true);
                break;
            case peer_comm_byte_INIT_DATA:
                peer_comm_set_incoming_dataset(pcc, b & peer_comm_byte_DATA_ID_MASK);
                peer_comm_emit_next_byte(pcc, false);
                break;
            case peer_comm_byte_PING:
                pcc->peer_ready = true;
                pcc->put(peer_comm_byte_READY);
                break;
            case peer_comm_byte_READY:
                pcc->peer_ready = true;
                break;
            case peer_comm_byte_INIT_CYCLE:
                peer_comm_reset_state(pcc);
                pcc->busy = true;
                peer_comm_emit_next_byte(pcc, false);
                break;
            case peer_comm_byte_DONE_CYCLE:
                pcc->busy = false;
                break;
            }
        } else { // received a data byte
            peer_comm_save_next_data_byte(pcc, b & peer_comm_byte_DATA_MASK);
            peer_comm_emit_next_byte(pcc, false);
        }
    }

#ifdef __cplusplus
}
#endif

#endif
