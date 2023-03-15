#include <string.h>

#include "peer_comm.h"

#define peer_comm_byte_DATA_MASK         0b01111111 // 7 bits for data

#define peer_comm_byte_SET_MSB           0b10000000 // command to send MSB bits
#define peer_comm_byte_MSB_MASK          0b00111111 // 6 bits for MSB bits of last six data bytes

#define peer_comm_byte_INIT_DATA         0b11000000 // command to select destination dataset
#define peer_comm_byte_DATA_ID_MASK      0b00011111 // 5 bits for buffer id

#define peer_comm_byte_VERSION           0b11100000 // command to send version
#define peer_comm_byte_VERSION_MASK      0b00001111 // 4 bits for version

#define peer_comm_byte_NO_DATA           0xF0
#define peer_comm_byte_SEEK_PEER         0xF1
#define peer_comm_byte_PEER_READY        0xF2
#define peer_comm_byte_SEEK_SLAVE        0xF3
#define peer_comm_byte_SLAVE_READY       0xF4
#define peer_comm_byte_INIT_CYCLE        0xF5
#define peer_comm_byte_END_CYCLE         0xF6
#define peer_comm_byte_END_DATA          0xF7

static void peer_comm_reset_state(peer_comm_config_t* pcc) {
    pcc->busy = false;
    pcc->in_active = false;
    pcc->out_stage = 0;
    pcc->last_state = 0;
}

peer_comm_config_t* new_peer_comm_config(uint8_t version,
                                         uint8_t size,
                                         shared_buffer_t** datasets,
                                         uint8_t* data_inits) {
    peer_comm_config_t* pcc = (peer_comm_config_t*) malloc(sizeof(peer_comm_config_t));
    pcc->version = version;
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
    pcc->role = peer_comm_role_NONE;
    peer_comm_reset_state(pcc);

    return pcc;
}

void peer_comm_set_handlers(peer_comm_config_t* pcc,
                            uint8_t (*get) (void), void (*put) (uint8_t), uint64_t (*current_ts) (void)) {
    pcc->get = get;
    pcc->put = put;
    pcc->current_ts = current_ts;
}

void free_peer_comm_config(peer_comm_config_t* pcc) {
    free(pcc->data_ts);
    free(pcc->data_inits);
    free(pcc->datasets);
    free(pcc);
}

uint8_t peer_comm_cmd_init_data(uint8_t data_id) {
    if(data_id>>5) return 0; // invalid id, max 5 bits
    return peer_comm_byte_INIT_DATA | data_id;
}

void peer_comm_init_cycle(peer_comm_config_t* pcc) {
    if(pcc->busy) {
        uint32_t current_state;
        uint8_t* p = (uint8_t*) &current_state;
        p[0] = pcc->in_id;
        p[1] = pcc->in_pos;
        p[2] = pcc->out_id;
        p[3] = pcc->out_pos;
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

void peer_comm_try_peer(peer_comm_config_t* pcc) {
    if(pcc->peer_ready) return;
    pcc->put(peer_comm_byte_SEEK_PEER);
}

void peer_comm_try_master(peer_comm_config_t* pcc, bool left) {
    if(pcc->role != peer_comm_role_NONE) return;
    pcc->role = left ? peer_comm_role_MASTER_LEFT : peer_comm_role_MASTER_RIGHT;
    pcc->put(peer_comm_byte_SEEK_SLAVE);
}

void peer_comm_try_version(peer_comm_config_t* pcc) {
    if(pcc->peer_version) return;
    pcc->put(peer_comm_byte_VERSION | pcc->version);
}

static uint8_t peer_comm_select_next_out(peer_comm_config_t* pcc, uint8_t start) {
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
static void peer_comm_emit_next_byte(peer_comm_config_t* pcc, bool peer_empty) {
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
            uint8_t pos = pcc->out_pos;
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
static void peer_comm_start_incoming_dataset(peer_comm_config_t* pcc, uint8_t id) {
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
static void peer_comm_save_next_data_byte(peer_comm_config_t* pcc, uint8_t d) {
    if(!pcc->in_active) return; // must be active

    if(pcc->in_pos < pcc->in_size) {
        pcc->in_buff[pcc->in_pos++] = d;
    }
}

static void peer_comm_set_msbs(peer_comm_config_t* pcc, uint8_t msbs) {
    if(!pcc->in_active) return; // must be active

    int pos = pcc->in_pos-1;
    for(; msbs && pos>=0; msbs>>=1, pos--) {
        if(msbs & 1) pcc->in_buff[pos] |= 0b10000000; // set the first bit
    }
}


/*
 * finish reception of current incoming dataset
 */
static void peer_comm_finish_incoming_dataset(peer_comm_config_t* pcc) {
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
    } else if(0 == (b & 0b00010000)) {
        // received version
        if(!pcc->peer_version) {
            pcc->peer_version = b & peer_comm_byte_VERSION_MASK;
            pcc->put(pcc->version);
        }
    } else {
        // other command byte
        switch(b) {
        case peer_comm_byte_SEEK_PEER: {
            pcc->peer_ready = true;
            pcc->put(peer_comm_byte_PEER_READY);
            break;
        }
        case peer_comm_byte_PEER_READY: {
            pcc->peer_ready = true;
            break;
        }
        case peer_comm_byte_SEEK_SLAVE: {
            if(pcc->role == peer_comm_role_NONE || pcc->role == peer_comm_role_MASTER_RIGHT)
                pcc->role = peer_comm_role_SLAVE;
            if(pcc->role == peer_comm_role_SLAVE)
                pcc->put(peer_comm_byte_SLAVE_READY);
            break;
        }
        case peer_comm_byte_SLAVE_READY: {
            pcc->role = peer_comm_role_MASTER;
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
