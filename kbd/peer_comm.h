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

#define peer_comm_byte_DATA_MASK         0b01111111 // 7 bits for data

#define peer_comm_byte_SET_MSB           0b10000000 // command to send MSB bits
#define peer_comm_byte_MSB_MASK          0b00111111 // 6 bits for MSB bits of last six data bytes

#define peer_comm_byte_INIT_DATA         0b11000000 // command to select destination dataset
#define peer_comm_byte_DATA_ID_MASK      0b00011111 // 5 bits for buffer id

#define peer_comm_byte_NO_DATA           0xF0
#define peer_comm_byte_PING              0xF1
#define peer_comm_byte_PEER_READY        0xF2
#define peer_comm_byte_INIT_CYCLE        0xF3
#define peer_comm_byte_END_CYCLE         0xF4
#define peer_comm_byte_END_DATA          0xF5

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

    void peer_comm_reset_state(peer_comm_config_t* pcc) {
        pcc->busy = false;
        pcc->in_active = false;
        pcc->out_stage = 0;
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
        pcc->data_ts = (uint64_t*) malloc(sizeof(uint64_t) * size);
        for(int i=0; i<size; i++) {
            pcc->datasets[i] = datasets[i];
            pcc->data_inits[i] = data_inits[i];
            pcc->data_ts[i] = 0;
        }

        pcc->peer_ready = false;
        peer_comm_reset_state(pcc);

        pcc->get = get;
        pcc->put = put;
        pcc->current_ts = current_ts;
        return pcc;
    }

    void free_peer_comm_config(peer_comm_config_t* pcc) {
        free(pcc->data_ts);
        free(pcc->data_inits);
        free(pcc->datasets);
        free(pcc);
    }

    /*
     * To be called by the master unit, periodically, to start one transfer cycle
     */
    void peer_comm_init_cycle(peer_comm_config_t* pcc) {
        if(pcc->busy) {
            uint32_t current_state;
            uint8_t* p = (uint8_t*) &current_state;
            p[0] = pcc->in_id;
            p[1] = pcc->in_pos;
            p[2] = pcc->out_id;
            p[2] = pcc->out_pos;
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
     *
     */
    uint8_t peer_comm_select_next_out(peer_comm_config_t* pcc, uint8_t start) {
        for(uint8_t id=start; id<pcc->size; id++) {
            shared_buffer_t* sb = pcc->datasets[id];
            uint8_t data_init = pcc->data_inits[id];
            if(data_init && sb->ts_start!=pcc->data_ts[id]) {
                read_shared_buffer(sb, &(pcc->data_ts[id]), pcc->out_buff);
                pcc->out_size = sb->size;
                pcc->out_id = id;
                pcc->out_pos = 0;
                pcc->msbs = 0;
                return data_init;
            }
        }
        return 0;
    }

    /*
     * Either initiate sending of next dataset out
     * or continue data byte for next pos of current dataset out
     * Clears busy flag when done and informs the other unit
     */
    void peer_comm_emit_next_byte(peer_comm_config_t* pcc, bool peer_empty) {
        switch(pcc->out_stage) {
        case 0: // select first outgoing dataset
        {
            uint8_t init_data_cmd = peer_comm_select_next_out(pcc, 0);
            if(init_data_cmd) {
                pcc->out_stage = 1;
                pcc->put(init_data_cmd);
            } else {
                pcc->out_stage = 4;
                pcc->put(peer_comm_byte_NO_DATA);
            }
            break;
        }
        case 1: // send data byte or MSB bits
        {
            if(pcc->msbs & 0b00100000) {
                uint8_t msbs = peer_comm_byte_SET_MSB | pcc->msbs;
                pcc->msbs = 0;
                pcc->put(msbs);
            } else {
                uint pos = pcc->out_pos;
                uint8_t d = pcc->out_buff[pos++];
                pcc->msbs <<= 1;
                if(d & 0b10000000) pcc->msbs |= 1;
                if(pos<pcc->out_size) {
                    pcc->out_pos = pos;
                } else {
                    pcc->out_stage = 2;
                }
                pcc->put(d);
            }
            break;
        }
        case 2: // end current outgoing dataset
        {
            if(pcc->msbs) {
                uint8_t msbs = peer_comm_byte_SET_MSB | pcc->msbs;
                pcc->msbs = 0;
                pcc->put(msbs);
            } else {
                pcc->out_stage = 3;
                pcc->put(peer_comm_byte_END_DATA);
            }
            break;
        }
        case 3: // select next outgoing dataset
        {
            uint8_t init_data_cmd = peer_comm_select_next_out(pcc, pcc->out_id);
            if(init_data_cmd) {
                pcc->out_stage = 1;
                pcc->put(init_data_cmd);
            } else {
                pcc->out_stage = 4;
                pcc->put(peer_comm_byte_NO_DATA);
            }
            break;
        }
        case 4: // end cycle
        {
            // i have no dataset left to send
            if(peer_empty) {
                // peer too has nothing left to send
                pcc->put(peer_comm_byte_END_CYCLE);
                pcc->busy = false;
            } else {
                // peer still has something left to send
                pcc->put(peer_comm_byte_NO_DATA);
            }
            break;
        }}
    }

    /*
     * prepare for next incoming dataset
     */
    void peer_comm_start_incoming_dataset(peer_comm_config_t* pcc, uint8_t id) {
        // id must point to a valid incoming dataset
        if(id<pcc->size && pcc->data_inits[id]==0) {
            memset(pcc->in_buff, 0, peer_comm_BUFF_SIZE);
            pcc->in_size = pcc->datasets[id]->size;
            pcc->in_id = id;
            pcc->in_pos = 0;
            pcc->in_active = true;
        }
    }

    /*
     * save the next byte and mark dataset finished when last pos is reached
     */
    void peer_comm_save_next_data_byte(peer_comm_config_t* pcc, uint8_t d) {
        if(!pcc->in_active) return; // must be active

        if(pcc->in_pos < pcc->in_size) {
            pcc->in_buff[pcc->in_pos++] = d;
        }
    }

    void peer_comm_set_msbs(peer_comm_config_t* pcc, uint8_t msbs) {
        if(!pcc->in_active) return; // must be active

        uint8_t mask = 1;
        int pos = pcc->in_pos-1;
        for(; mask <= 0b00100000 && pos>=0; mask<<=1, pos--) {
            if(msbs & mask) pcc->in_buff[pos] |= 0b10000000; // set the first bit
        }
    }

    /*
     * finish reception of current incoming dataset
     */
    void peer_comm_finish_incoming_dataset(peer_comm_config_t* pcc) {
        if(!pcc->in_active) return; // must be active
        pcc->in_active = false;

        uint8_t id = pcc->in_id;
        shared_buffer_t* sb = id<pcc->size && pcc->data_inits[id]==0 ? pcc->datasets[id] : NULL;

        // move out the data from local temp buffer to target dataset
        if(sb) {
            write_shared_buffer(sb, pcc->current_ts(), pcc->in_buff);
        }
    }

    void peer_comm_on_receive(peer_comm_config_t* pcc) {
        uint8_t b = pcc->get(); // get the received byte

        if(0 == (b & 0b10000000)) {
            // received a data byte
            uint8_t d = b & peer_comm_byte_DATA_MASK;
            peer_comm_save_next_data_byte(pcc, d);
            peer_comm_emit_next_byte(pcc, false);
        } else if(0 == (b & 0b01000000)) {
            // received msb bits
            uint8_t msbs = b & peer_comm_byte_MSB_MASK;
            peer_comm_set_msbs(pcc, msbs);
            peer_comm_emit_next_byte(pcc, false);
        } else if(0 == (b & 0b00100000)) {
            // received dataset selector INIT_DATA
            uint8_t id = b & peer_comm_byte_DATA_ID_MASK;
            peer_comm_start_incoming_dataset(pcc, id);
            peer_comm_emit_next_byte(pcc, false);
        } else {
            // other command byte
            switch(b) {
            case peer_comm_byte_PING: {
                pcc->peer_ready = true;
                pcc->put(peer_comm_byte_PEER_READY);
                break;
            }
            case peer_comm_byte_PEER_READY: {
                pcc->peer_ready = true;
                break;
            }
            case peer_comm_byte_INIT_CYCLE: {
                peer_comm_reset_state(pcc);
                pcc->busy = true;
                peer_comm_emit_next_byte(pcc, false);
                break;
            }
            case peer_comm_byte_END_CYCLE: {
                pcc->busy = false;
                break;
            }
            case peer_comm_byte_END_DATA: {
                peer_comm_finish_incoming_dataset(pcc);
                peer_comm_emit_next_byte(pcc, false);
                break;
            }
            case peer_comm_byte_NO_DATA:
            default: {
                peer_comm_emit_next_byte(pcc, true);
                break;
            }}
        }
    }

#ifdef __cplusplus
}
#endif

#endif
