#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_model.h"


kbd_system_t kbd_system = {
    .core0 = { /// CORE 0 ///
#ifdef KBD_NODE_AP
        .left_key_press = {0},  // default to 0
        .right_key_press = {0}, // default to 0
#endif

#if defined(KBD_NODE_AP) || defined(KBD_NODE_RIGHT)
        .tb_motion_ts = 0,
        .tb_motion = {}, // default to 0
#endif

#ifdef KBD_NODE_AP
        .hid_report_in = {},  // default to 0
        .hid_report_out = {}, // default to 0

        .left_task_request_ts = 0,
        .left_task_request = {0},   // default to 0
        .left_task_response_ts = 0,
        .left_task_response = {0},  // default to 0

        .right_task_request_ts = 0,
        .right_task_request = {0},   // default to 0
        .right_task_response_ts = 0,
        .right_task_response = {0},  // default to 0
#endif

        .task_request_ts = 0,
        .task_request = {0},   // default to 0
        .task_response_ts = 0,
        .task_response = {0},  // default to 0

#ifdef KBD_NODE_AP
        .active_ts = 0,
        .idle_minutes = 0,
#endif

#ifdef KBD_NODE_LEFT
        .date = {2000, 1, 1, 1, 0, 0, 0},
        .temperature = 0,
#endif

        .tb_config = {
            .cpi = 1600,
            .scroll_scale = 32,
            .scroll_quad_weight = 0,
            .delta_scale = 4,
            .delta_quad_weight = 0,
        },
        .pixel_config = {
            .color = 0x0f000f, // magenta
            .anim_style = pixel_anim_style_FADE, // fade
            .anim_cycles = 30 // not applicable when fixed
        }

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
        .pixel_colors = {0}, // default to 0
#endif

#ifdef KBD_NODE_AP
        .flash_datasets = {0}, // default to NULL
#else
        .flash_data = {}, // default to 0
        .flash_data_pos = {}, // default to 0
#endif

        .state_ts = 0,
        .state = {
            .screen = kbd_info_screen_welcome,
            .usb_hid_state = kbd_usb_hid_state_UNMOUNTED,
            .flags = KBD_FLAGS_PIXELS_ON,
            .backlight = 30
        }
    },

    .core1 = { /// CORE 1 ///
        .tcp_server = {},
        .udp_server = {
            .recv_size = {0};
            .send_size = {0};
        }
        .reboot = true,
#ifdef KBD_NODE_AP
        .dhcp_server = {},
#endif

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
        .key_press = {0},    // default to 0
#endif
    },

    .pixels_on = true,
    .screen = kbd_info_screen_welcome,
    .usb_hid_state = kbd_usb_hid_state_UNMOUNTED,
    .comm_state = kbd_comm_state_init,

#if defined(KBD_NODE_AP) || defined(KBD_NODE_LEFT)
    .backlight = 30, // 30%
#endif

#ifdef KBD_NODE_AP
    .led_left = kbd_led_state_ON,
    .led_right = kbd_led_state_ON,
#else
    .led = kbd_led_state_ON,
#endif

    .ledB = kbd_led_state_ON,

#ifdef KBD_NODE_AP
    .left_active = false,
    .right_active = false,
#else
    .ap_connected = false,
#endif

    .sb_state = NULL,

#ifdef KBD_NODE_AP
    .sb_left_key_press = NULL,
    .sb_right_key_press = NULL,
#endif

#ifdef KBD_NODE_AP
    .sb_left_task_request = NULL,
    .sb_left_task_response = NULL,
    .sb_right_task_request = NULL,
    .sb_right_task_response = NULL,
#else
    .sb_task_request = NULL,
    .sb_task_response = NULL,
#endif

#if defined(KBD_NODE_AP) || defined(KBD_NODE_RIGHT)
    .sb_tb_motion = NULL,
#endif

    .spin_lock = NULL
};


void init_data_model() {
    int spin_lock_num = spin_lock_claim_unused(true);
    spin_lock_t* spin_lock = spin_lock_init(spin_lock_num);

    kbd_system.spin_lock = spin_lock;

    SET_MY_IP4_ADDR(&kbd_system.core1.tcp_server->gw);

    kbd_system.sb_state = new_shared_buffer(sizeof(kbd_state_t), spin_lock);
    write_shared_buffer(kbd_system.sb_state, kbd_system.state_ts, &kbd_system.core0.state);

#ifdef KBD_NODE_AP
    kbd_system.sb_left_key_press = new_shared_buffer(KEY_ROW_COUNT, spin_lock);  // 1 byte per row
    kbd_system.sb_right_key_press = new_shared_buffer(KEY_ROW_COUNT, spin_lock); // 1 byte per row
#endif

#ifdef KBD_NODE_AP
    kbd_system.sb_left_task_request = new_shared_buffer(KBD_TASK_SIZE, spin_lock);
    kbd_system.sb_left_task_response = new_shared_buffer(KBD_TASK_SIZE, spin_lock);
    kbd_system.sb_right_task_request = new_shared_buffer(KBD_TASK_SIZE, spin_lock);
    kbd_system.sb_right_task_response = new_shared_buffer(KBD_TASK_SIZE, spin_lock);
#else
    kbd_system.sb_task_request = new_shared_buffer(KBD_TASK_SIZE, spin_lock);
    kbd_system.sb_task_response = new_shared_buffer(KBD_TASK_SIZE, spin_lock);
#endif

#if defined(KBD_NODE_AP) || defined(KBD_NODE_RIGHT)
    kbd_system.sb_tb_motion = new_shared_buffer(sizeof(kbd_tb_motion_t), spin_lock);
#endif

}
