#ifndef _DATA_MODEL_H
#define _DATA_MODEL_H

#include "peer_comm.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Tasks (10ms process cycle)
     *
     * core-1: scan key_press                   core-0: scan ball_scroll (right)
     *         UART TX/RX                               update state (master)
     *                                                  set tasks (master)
     *                                                  execute task (master & slave)
     *                                                  prepare hid report and send (master)
     *                                                  process state->led (left & right)
     *                                                  process state->lcd (left)
     *
     * Flow (left-master, right-slave)
     *
     * core-1
     * scan : left:scan ==> left:left_key_press
     *        right:scan ==> right:right_key_press
     * TX/RX: right:right_key_press ==> left:right_key_press
     *        right:right_ball_scroll ==> left:right_ball_scroll
     *        left:system_state ==> right:system_state
     *        left:right_task_requests ==> right:right_task_requests
     *        right:right_task_responses ==> left:right_task_responses
     *
     * core-0
     * scan   : right:scan ==> right:right_input->ball_scroll
     * Process: left:left_key_press   ==>  left:system_state
     *          left:right_key_press       left:hid_report
     *          left:right_ball_scroll
     *          left:usb_host_input
     *          left:left_task_responses
     *          left:right_task_responses
     *
     *          left:hid_report ==> left:usb
     *          left:usb ==> left:usb_host_input
     *
     *          left:left_task_requests ==> left:right_task_responses
     *          right:right_task_requests ==> right:right_task_responses
     *
     *          right:system_state ==> right:led (caps_lock)
     *          left:system_state ==> left:lcd
     *                                left:led (status)
     */

    typedef enum {
        kbd_side_NONE = 0,
        kbd_side_LEFT = 1,
        kbd_side_RIGHT = 2
    } kbd_side_t;

    typedef enum {
        kbd_side_role_NONE = 0,
        kbd_side_role_MASTER = 1, // primary unit talking to the USB host
        kbd_side_role_SLAVE = 2  // secondary unit talking to the primary unit
    } kbd_side_role_t;

    /*
     * Global data
     */

    struct {
        kbd_side_t side;
        kbd_side_role_t side_role;

        bool ready;

        shared_buffer_t* left_key_press;
        shared_buffer_t* right_key_press;
        shared_buffer_t* right_ball_scroll;
        shared_buffer_t* system_state;
        shared_buffer_t* left_task_requests;
        shared_buffer_t* right_task_requests;
        shared_buffer_t* left_task_responses;
        shared_buffer_t* right_task_responses;

        peer_comm_config_t* peer_comm;
    } my_data = {
        .side = kbd_side_NONE,
        .side_role = kbd_side_role_NONE,
        .ready = false,
        .left_key_press = NULL,
        .right_key_press = NULL,
        .right_ball_scroll = NULL,
        .system_state = NULL,
        .left_task_requests = NULL,
        .right_task_requests = NULL,
        .left_task_responses = NULL,
        .right_task_responses = NULL,
        .peer_comm = NULL
    };

    /*
     * Model functions
     */

    void setup_data_model(uint8_t (*get) (void),
                          void (*put) (uint8_t),
                          uint64_t (*current_ts) (void)) {
        my_data.left_key_press = new_shared_buffer(6);  // 6x7 buttons
        my_data.right_key_press = new_shared_buffer(6); // 6x7 buttons
        my_data.right_ball_scroll = new_shared_buffer(4); // dx(16bit), dy(16bit)

        // TBD: actual size, assuming 8 bytes for now
        my_data.system_state = new_shared_buffer(8);
        my_data.left_task_requests = new_shared_buffer(8);
        my_data.right_task_requests = new_shared_buffer(8);
        my_data.left_task_responses = new_shared_buffer(8);
        my_data.right_task_responses = new_shared_buffer(8);

        shared_buffer_t* sbs[8] = {
            my_data.system_state,        // DATA_ID: 0
            my_data.left_key_press,      //          1
            my_data.right_key_press,     //          2
            my_data.right_ball_scroll,   //          3
            my_data.left_task_requests,  //          4
            my_data.right_task_requests, //          5
            my_data.left_task_responses, //          6
            my_data.right_task_responses //          7
        };
        uint8_t data_inits[8] = {0, 0, 0, 0,  0, 0, 0, 0};
        my_data.peer_comm = new_peer_comm_config(8, sbs, data_inits, get, put, current_ts);
    }

    void setup_role(kbd_side_role_t role) {
        uint8_t* dis = my_data.peer_comm->data_inits;
        my_data.side_role = role;
        if(my_data.side==kbd_side_LEFT) {
            if(role==kbd_side_role_MASTER) {
                dis[0] = peer_comm_byte_INIT_DATA | 0;
                dis[5] = peer_comm_byte_INIT_DATA | 5;
            } else { // SLAVE
                dis[1] = peer_comm_byte_INIT_DATA | 1;
                dis[6] = peer_comm_byte_INIT_DATA | 6;
            }
        } else { // RIGHT
            if(role==kbd_side_role_MASTER) {
                dis[0] = peer_comm_byte_INIT_DATA | 0;
                dis[4] = peer_comm_byte_INIT_DATA | 4;
            } else { // SLAVE
                dis[2] = peer_comm_byte_INIT_DATA | 2;
                dis[3] = peer_comm_byte_INIT_DATA | 3;
                dis[7] = peer_comm_byte_INIT_DATA | 7;
            }
        }
    }

#ifdef __cplusplus
}
#endif

#endif
