#ifndef _DATA_MODEL_H
#define _DATA_MODEL_H

#include "peer_comm.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Global data
     *
     * All data bytes in shared_buffer are 7bit data.
     * The first bit is reserved for command bytes for use with UART transmission.
     */

    typedef struct {
        shared_buffer_t* key_press;
        shared_buffer_t* ball_scroll;
    } device_input_t;

    typedef enum {
        device_side_LEFT = 0,
        device_side_RIGHT = 1
    } device_side_t;

    typedef enum {
        device_role_USB = 0, // primary unit talking to the USB host
        device_role_AUX = 1  // secondary unit talking to the primary unit
    } device_role_t;

    device_side_t my_side;
    device_role_t my_role;

    device_input_t* left_input;
    device_input_t* right_input;

    shared_buffer_t* system_state;

    peer_comm_config_t* serial_transfer;

    device_input_t* new_device_input(void) {
        device_input_t* di = (device_input_t*) malloc(sizeof(device_input_t));

        // Input data memory allocation
        // 1 bit -> 1 key, 1 byte can keep 7 bits
        // 7 keyboard rows each with max 7 btns -> 7 bytes (7x7 bits)
        // 1 trackball btn -> 1 byte (upto 7 bits)
        di->key_press = new_shared_buffer(8); // 0-6: keyboard btns, 7: trackball btn
        // 4 trackball scrolls (up, down, left, right)
        // 2 byte for each would allow range of 0-16129 (we use 7bits only)
        // trackball scroll value is absolute and would keep increasing
        // absolute would make transmission safe against any missed ones
        // once it reaches max, start again from 0
        // the master process would identify the decrease as overflow and calc the correct increment
        // the range is large enough to last several process cycles
        di->ball_scroll = new_shared_buffer(8);

        return di;
    }

    void free_device_input(device_input_t* di) {
        free(di->key_press);
        free(di->ball_scroll);
        free(di);
    }

    void setup_data_model(uint8_t (*get) (void),
                          void (*put) (uint8_t),
                          uint64_t (*current_ts) (void)) {
        left_input = new_device_input();
        right_input = new_device_input();

        // TBD: actual size, assuming 8 bytes for now
        system_state = new_shared_buffer(8);

        shared_buffer_t* sbs[5] = {system_state,
                                   left_input->key_press,
                                   left_input->ball_scroll,
                                   right_input->key_press,
                                   right_input->ball_scroll};
        uint8_t data_inits[5] = {0, 0, 0, 0, 0};
        serial_transfer = new_peer_comm_config(5, sbs, data_inits, get, put, current_ts);
    }

    void setup_side_and_role(device_side_t side, device_role_t role) {
        uint8_t* dis = serial_transfer->data_inits;
        if(side==device_side_LEFT) {
            if(role==device_role_USB) {
                dis[0] = peer_comm_byte_INIT_DATA | 0;
            } else {
                dis[1] = peer_comm_byte_INIT_DATA | 1;
            }
        } else {
            if(role==device_role_AUX) {
                dis[0] = peer_comm_byte_INIT_DATA | 0;
            } else {
                dis[3] = peer_comm_byte_INIT_DATA | 3;
                dis[4] = peer_comm_byte_INIT_DATA | 4;
            }
        }
    }

    /*
     * Tasks (10ms process cycle)
     *
     * core-1: scan key_press                   core-0: update state (master)
     *         irq ball_scroll (right)                  prepare report (master)
     *         transmit input (slave)                   communicate with usb host (master)
     *         receive slave input (master)             process state->led (right)
     *         transmit state (master)                  process state->lcd (left)
     *         receive state output (slave)
     *
     * Flow (left-master, right-slave)
     *
     * core-1
     * scan/irq : left:scan ==> left:left_input->key_press->buff_in
     *            left:irq ==> left:left_input->ball_scroll->buff_in
     *            right:scan ==> right:right_input->key_press->buff_in
     *            right:irq ==> right:right_input->ball_scroll->buff_in
     *
     * TX/RX : right:right_input->key_press->buff_out ==> left:right_input->key_press->buff_in
     *         right:right_input->ball_scroll->buff_out ==> left:right_input->key_press->buff_in
     *         left:system_state->buff_out ==> right:system_state->buff_in
     *
     * core-0
     * Process: left:left_input->key_press->buff_out   ==>  left:system_state->buff_in
     *          left:left_input->ball_scroll->buff_out      left:hid_report
     *          left:right_input->key_press->buff_out
     *          left:right_input->ball_scroll->buff_out
     *          left:usb_host_input
     *
     *          left:hid_report ==> left:usb
     *          left:usb ==> left:usb_host_input
     *
     *          // to read system_state, master can use buff_in since written by itself
     *          // whereas slave can read buff_out since it was written by core-1 TX/RX
     *          right:system_state->buff_out ==> right:led (trackball, caps_lock)
     *          left:system_state->buff_in ==> left:lcd
     */

#ifdef __cplusplus
}
#endif

#endif
