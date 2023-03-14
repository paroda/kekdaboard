#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_model.h"
#include "hw_config.h"

kbd_screen_t kbd_info_screens[KBD_INFO_SCREEN_COUNT] = {
    kbd_info_screen_home,
};

kbd_screen_t kbd_config_screens[KBD_CONFIG_SCREEN_COUNT] = {
    kbd_config_screen_date,
};

kbd_system_t kbd_system = {
    .side = kbd_side_NONE,
    .role = kbd_role_NONE,
    .ready = false,
    .state_ts = 0,
    .state.caps_lock = false,
    .state.screen = kbd_info_screen_home,
    .sb_state = NULL,
    .sb_left_key_press = NULL,
    .sb_right_key_press = NULL,
    .sb_right_tb_motion = NULL,
    .sb_left_task_request = NULL,
    .sb_right_task_request = NULL,
    .sb_left_task_response = NULL,
    .sb_right_task_response = NULL,
    .usb_hid_state = kbd_usb_hid_state_UNMOUNTED,
    .led = kbd_led_state_OFF,
    .ledB = kbd_led_state_OFF,
    .comm = NULL
};

void init_data_model() {
    kbd_system.sb_state = new_shared_buffer(sizeof(kbd_state_t));
    write_shared_buffer(kbd_system.sb_state, kbd_system.state_ts, (uint8_t*)&kbd_system.state);

    kbd_system.sb_left_key_press = new_shared_buffer(KEY_ROW_COUNT);  // 1 byte per row
    kbd_system.sb_right_key_press = new_shared_buffer(KEY_ROW_COUNT); // 1 byte per row
    kbd_system.sb_right_tb_motion = new_shared_buffer(sizeof(kbd_tb_motion_t));
    kbd_system.sb_left_task_request = new_shared_buffer(32);
    kbd_system.sb_right_task_request = new_shared_buffer(32);
    kbd_system.sb_left_task_response = new_shared_buffer(32);
    kbd_system.sb_right_task_response = new_shared_buffer(32);

    memset((void*)&(kbd_system.hid_report_in), 0, sizeof(hid_report_in_t));
    memset((void*)&(kbd_system.hid_report_out), 0, sizeof(hid_report_out_t));

    shared_buffer_t* sbs[KBD_SB_COUNT] = {
        kbd_system.sb_state,               // DATA_ID: 0
        kbd_system.sb_left_key_press,      //          1
        kbd_system.sb_right_key_press,     //          2
        kbd_system.sb_right_tb_motion,     //          3
        kbd_system.sb_left_task_request,   //          4
        kbd_system.sb_right_task_request,  //          5
        kbd_system.sb_left_task_response,  //          6
        kbd_system.sb_right_task_response  //          7
    };
    uint8_t data_inits[KBD_SB_COUNT] = {0, 0, 0, 0,  0, 0, 0, 0};
    kbd_system.comm = new_peer_comm_config(KBD_SB_COUNT, sbs, data_inits);
}

void set_kbd_side(kbd_side_t side) {
    kbd_system.side = side;
}

void set_kbd_role(kbd_role_t role) {
    uint8_t* dis = kbd_system.comm->data_inits;
    kbd_system.role = role;
    // define which datasets are to be sent out to the peer
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
