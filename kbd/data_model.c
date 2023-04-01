#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_model.h"

kbd_system_t kbd_system = {
    .version = 0,

    .side = kbd_side_NONE,
    .role = kbd_role_NONE,

    .ready = false,

    .state_ts = 0,
    .sb_state = NULL,

    .left_task_request_ts = 0,
    .right_task_request_ts = 0,
    .left_task_response_ts = 0,
    .right_task_response_ts = 0,

    .sb_left_key_press = NULL,
    .sb_right_key_press = NULL,
    .sb_right_tb_motion = NULL,
    .sb_left_task_request = NULL,
    .sb_right_task_request = NULL,
    .sb_left_task_response = NULL,
    .sb_right_task_response = NULL,

    .backlight = 30, // 30%
    .screen = kbd_info_screen_welcome,
    .usb_hid_state = kbd_usb_hid_state_UNMOUNTED,

    .active_ts = 0,
    .idle_minutes = 5,

    .tb_cpi = 1600,
    .tb_scroll_scale = 32,
    .tb_scroll_quad_weight = 2,
    .tb_delta_scale = 4,
    .tb_delta_quad_weight = 2,

    .date = {2000, 1, 1, 1, 0, 0, 0},
    .temperature = 0,

    .led = kbd_led_state_OFF,
    .ledB = kbd_led_state_OFF,

    .comm = NULL,
    .spin_lock = NULL
};

void init_data_model() {
    int spin_lock_num = spin_lock_claim_unused(true);
    spin_lock_t* spin_lock = spin_lock_init(spin_lock_num);

    kbd_system.spin_lock = spin_lock;

    kbd_system.sb_state = new_shared_buffer(sizeof(kbd_state_t), spin_lock);
    kbd_system.sb_left_key_press = new_shared_buffer(KEY_ROW_COUNT, spin_lock);  // 1 byte per row
    kbd_system.sb_right_key_press = new_shared_buffer(KEY_ROW_COUNT, spin_lock); // 1 byte per row
    kbd_system.sb_right_tb_motion = new_shared_buffer(sizeof(kbd_tb_motion_t), spin_lock);
    kbd_system.sb_left_task_request = new_shared_buffer(KBD_TASK_REQUEST_SIZE, spin_lock);
    kbd_system.sb_right_task_request = new_shared_buffer(KBD_TASK_REQUEST_SIZE, spin_lock);
    kbd_system.sb_left_task_response = new_shared_buffer(KBD_TASK_RESPONSE_SIZE, spin_lock);
    kbd_system.sb_right_task_response = new_shared_buffer(KBD_TASK_RESPONSE_SIZE, spin_lock);

    memset(&kbd_system.state, 0, sizeof(kbd_state_t));
    kbd_system.state.screen = kbd_info_screen_welcome;
    kbd_system.state.usb_hid_state = kbd_usb_hid_state_UNMOUNTED;
    kbd_system.state.caps_lock = false;
    kbd_system.state.num_lock = false;
    kbd_system.state.scroll_lock = false;
    write_shared_buffer(kbd_system.sb_state, kbd_system.state_ts, &kbd_system.state);

    memset(kbd_system.left_key_press, 0, KEY_ROW_COUNT);
    memset(kbd_system.right_key_press, 0, KEY_ROW_COUNT);
    memset(&kbd_system.right_tb_motion, 0, sizeof(kbd_tb_motion_t));

    memset(&kbd_system.hid_report_in, 0, sizeof(hid_report_in_t));
    memset(&kbd_system.hid_report_out, 0, sizeof(hid_report_out_t));

    memset(kbd_system.left_task_request, 0, KBD_TASK_REQUEST_SIZE);
    memset(kbd_system.right_task_request, 0, KBD_TASK_REQUEST_SIZE);
    memset(kbd_system.left_task_response, 0, KBD_TASK_RESPONSE_SIZE);
    memset(kbd_system.right_task_response, 0, KBD_TASK_RESPONSE_SIZE);

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
    kbd_system.comm = new_peer_comm_config(KBD_VERSION, KBD_SB_COUNT, sbs, data_inits);
}

// to be called after loading the flash_header data
void init_kbd_side() {
    kbd_system.side = (kbd_system.flash_header[KBD_FLASH_ADDR_SIDE] == KBD_FLASH_SIDE_LEFT) ?
        kbd_side_LEFT : kbd_side_RIGHT;
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
