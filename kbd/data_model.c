#include "data_model.h"

kbd_system_t kbd_system = {
    .side = kbd_side_NONE,
    .role = kbd_role_NONE,
    .ready = false,
    .left_key_press = NULL,
    .right_key_press = NULL,
    .right_ball_scroll = NULL,
    .system_state = NULL,
    .left_task_request = NULL,
    .right_task_request = NULL,
    .left_task_response = NULL,
    .right_task_response = NULL,
    .tud_state = kbd_tud_state_UNMOUNTED,
    .comm = NULL
};

void init_data_model() {
    kbd_system.left_key_press = new_shared_buffer(6);  // 6x7 buttons
    kbd_system.right_key_press = new_shared_buffer(6); // 6x7 buttons
    kbd_system.right_ball_scroll = new_shared_buffer(4); // dx(16bit), dy(16bit)

    // TBD: actual size, assuming 32 bytes for now
    kbd_system.system_state = new_shared_buffer(32);
    kbd_system.left_task_request = new_shared_buffer(32);
    kbd_system.right_task_request = new_shared_buffer(32);
    kbd_system.left_task_response = new_shared_buffer(32);
    kbd_system.right_task_response = new_shared_buffer(32);

    shared_buffer_t* sbs[8] = {
        kbd_system.system_state,        // DATA_ID: 0
        kbd_system.left_key_press,      //          1
        kbd_system.right_key_press,     //          2
        kbd_system.right_ball_scroll,   //          3
        kbd_system.left_task_request,  //          4
        kbd_system.right_task_request, //          5
        kbd_system.left_task_response, //          6
        kbd_system.right_task_response //          7
    };
    uint8_t data_inits[8] = {0, 0, 0, 0,  0, 0, 0, 0};
    kbd_system.comm = new_peer_comm_config(8, sbs, data_inits);
}

void set_kbd_side(kbd_side_t side) {
    kbd_system.side = side;
}

void set_kbd_role(kbd_role_t role) {
    uint8_t* dis = kbd_system.comm->data_inits;
    kbd_system.role = role;
    if(kbd_system.side==kbd_side_LEFT) {
        if(role==kbd_role_MASTER) {
            dis[0] = peer_comm_cmd_init_data(0);
            dis[5] = peer_comm_cmd_init_data(5);
        } else { // SLAVE
            dis[1] = peer_comm_cmd_init_data(1);
            dis[6] = peer_comm_cmd_init_data(6);
        }
    } else { // RIGHT
        if(role==kbd_role_MASTER) {
            dis[0] = peer_comm_cmd_init_data(0);
            dis[4] = peer_comm_cmd_init_data(4);
        } else { // SLAVE
            dis[2] = peer_comm_cmd_init_data(2);
            dis[3] = peer_comm_cmd_init_data(3);
            dis[7] = peer_comm_cmd_init_data(7);
        }
    }
}
