#include "data_model.h"

kbd_system_t kbd_system = {
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

void setup_data_model(uint8_t (*get) (void),
                      void (*put) (uint8_t),
                      uint64_t (*current_ts) (void)) {
    kbd_system.left_key_press = new_shared_buffer(6);  // 6x7 buttons
    kbd_system.right_key_press = new_shared_buffer(6); // 6x7 buttons
    kbd_system.right_ball_scroll = new_shared_buffer(4); // dx(16bit), dy(16bit)

    // TBD: actual size, assuming 8 bytes for now
    kbd_system.system_state = new_shared_buffer(8);
    kbd_system.left_task_requests = new_shared_buffer(8);
    kbd_system.right_task_requests = new_shared_buffer(8);
    kbd_system.left_task_responses = new_shared_buffer(8);
    kbd_system.right_task_responses = new_shared_buffer(8);

    shared_buffer_t* sbs[8] = {
        kbd_system.system_state,        // DATA_ID: 0
        kbd_system.left_key_press,      //          1
        kbd_system.right_key_press,     //          2
        kbd_system.right_ball_scroll,   //          3
        kbd_system.left_task_requests,  //          4
        kbd_system.right_task_requests, //          5
        kbd_system.left_task_responses, //          6
        kbd_system.right_task_responses //          7
    };
    uint8_t data_inits[8] = {0, 0, 0, 0,  0, 0, 0, 0};
    kbd_system.peer_comm = new_peer_comm_config(8, sbs, data_inits, get, put, current_ts);
}

void setup_role(kbd_side_role_t role) {
    uint8_t* dis = kbd_system.peer_comm->data_inits;
    kbd_system.side_role = role;
    if(kbd_system.side==kbd_side_LEFT) {
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
