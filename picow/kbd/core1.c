#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/cyw43_arch.h"

#include "pico_fota_bootloader.h"

#include "hw_model.h"
#include "data_model.h"

static void toggle_led(void* param) {
    kbd_led_t* led = (kbd_led_t*) param;
    led->on = !led->on;
    if(led->wl_led)
        cyw43_arch_gpio_put(led->gpio, led->on);
    else
        gpio_put(led->gpio, led->on);
}

static void led_task() {
#ifdef KBD_NODE_AP
    static uint32_t led_left_last_ms = 0;
    int32_t led_left_ms = led_millis_to_toggle(&kbd_hw.led_left, kbd_system.led_left);
    if(led_left_ms>=0) do_if_elapsed(&led_left_last_ms, led_left_ms, &kbd_hw.led_left, toggle_led);

    static uint32_t led_right_last_ms = 0;
    int32_t led_right_ms = led_millis_to_toggle(&kbd_hw.led_right, kbd_system.led_right);
    if(led_right_ms>=0) do_if_elapsed(&led_right_last_ms, led_right_ms, &kbd_hw.led_right, toggle_led);
#else
    static uint32_t led_last_ms = 0;
    int32_t led_ms = led_millis_to_toggle(&kbd_hw.led, kbd_system.led);
    if(led_ms>=0) do_if_elapsed(&led_last_ms, led_ms, &kbd_hw.led, toggle_led);
#endif

    static uint32_t ledB_last_ms = 0;
    int32_t ledB_ms = led_millis_to_toggle(&kbd_hw.ledB, kbd_system.ledB);
    if(ledB_ms>=0) do_if_elapsed(&ledB_last_ms, ledB_ms, &kbd_hw.ledB, toggle_led);
}

static bool update_comm_state(kbd_comm_state_t* comm_state,
                              uint8_t* recv_buf, uint8_t* recv_size,
                              uint8_t* send_buf, uint8_t* send_size) {
    // in the absence of peer data, just continue as is
    if(*recv_size>0) {
        kbd_comm_state_t peer_state = (kbd_comm_state_t) recv_buf[0];
        switch(*comm_state) {
        case kbd_comm_state_init:
            if(peer_state==kbd_comm_state_init || peer_state==kbd_comm_state_ready) {
                *comm_state = kbd_comm_state_ready;
            }
            break;
        case kbd_comm_state_ready:
            if(peer_state!=kbd_comm_state_init) {
                *comm_state = kbd_comm_state_data;
            }
            break;
        case kbd_comm_state_data:
            if(peer_state==kbd_comm_state_init) {
                *comm_state = kbd_comm_state_reset;
            }
            break;
        default: // kbd_comm_state_reset
            // wait for core0 to update it to init
            break;
        }
    }
    if(*comm_state != kbd_comm_state_reset) {
        send_size = 1;
        send_buf[0] = *comm_state;
    }
}

typedef enum {
    comm_data_type_system_state = 0,
    comm_data_type_key_press,
    comm_data_type_tb_motion,
    comm_data_type_task_reqeust,
    comm_data_type_task_response,
} comm_data_type_t;

#ifdef KBD_NODE_AP

static void comm_task(void* param) {
    (void) param;
    for(uint8_t index=0; index<2; index++) {
        kbd_comm_state_t* comm_state = kbd_system.comm_state+index;
        uint8_t* recv_buf = server->recv_buf[index];
        uint8_t* recv_size = server->recv_size+index;
        uint8_t* send_buf = server->send_buf[index];
        uint8_t* send_size = server->send_size+index;
        if(*recv_size>0) {
            update_comm_state(comm_state, recv_buf, recv_size, send_buf, send_size);
            if(*comm_state==kbd_comm_state_data) {
                // recv: + key_press, tb_motion(right only), task_response
                int bytes_left = recv_size-1;
                uint8_t* buf = recv_buf+1;
                while(bytes_left>0) {
                    shared_buffer_t* sb = NULL;
                    switch(buf[0]) {
                    case comm_data_type_key_press:
                        sb = index==0 ? kbd_sytem.sb_left_key_press kbd_system.sb_right_key_press;
                        break;
                    case comm_data_type_tb_motion:
                        sb = kbd_system.sb_tb_motion;
                        break;
                    case comm_data_type_task_response:
                        sb = index==0 ? kbd_system.sb_left_task_response : kbd_system.sb_right_task_response;
                        break;
                    default: break;
                    }
                    if(sb) {
                        write_shared_buffer(sb, time_us_64(), buf+1);
                        bytes_left -= (1 + sb->size);
                        buf += (1 + sb->size);
                    } else {
                        bytes_left = 0; // stop if encountered invalid
                    }
                }
                *recv_size = 0;
                // send: + system_state, task_request
                comm_data_type_t types[2] = { comm_data_type_system_state, comm_data_type_task_reqeust };
                shared_buffer_t* sbs[2] = {
                    kbd_system.sb_state,
                    index==0 ? kbd_system.sb_left_task_request kbd_system.sb_right_task_request
                };
                static uint64_t task_ts[2] = {0,0}; // left,right
                uint64_t ts[2] = {
                    0,
                    task_ts[index]
                };
                buf = send_buf + *send_size;
                for(uint8_t i=0; i<2; i++) {
                    shared_buffer_t* sb = sbs[i];
                    if(sb->ts > ts[i]) {
                        buf[0] = types[i];
                        read_shared_buffer(sb, ts+i, buf+1);
                        *send_size += (1 + sb->size);
                        buf += (1 + sb->size);
                    }
                }
                task_ts[index] = ts[1];
            }
        }
    }
}

#else

static void key_scan_task(void* param) {
    (void)param;
    // scan key presses with kbd_hw.ks and save to core1.key_press
    key_scan_update(kbd_hw.ks);
    memcpy(kbd_system.core1.key_press, kbd_hw.ks->keys, hw_row_count);
}

static void comm_task(void* param) {
    (void) param;

#ifdef KBD_NODE_LEFT
    uint8_t index = 0;
#else
    uint8_t index = 1;
#endif

    kbd_comm_state_t* comm_state = kbd_system.comm_state+index;
    uint8_t* recv_buf = server->recv_buf[index];
    uint8_t* recv_size = server->recv_size+index;
    uint8_t* send_buf = server->send_buf[index];
    uint8_t* send_size = server->send_size+index;
    update_comm_state(comm_state, recv_buf, recv_size, send_buf, send_size);
    if(*comm_state==kbd_comm_state_data) {
        // recv: + system_state, task_request
        int bytes_left = recv_size-1;
        uint8_t* buf = recv_buf+1;
        while(byes_left>0) {
            shared_buffer_t* sb = NULL;
            switch(buff[0]) {
            case comm_data_type_system_state:
                sb = kbd_system.sb_state;
                break;
            case comm_data_type_task_reqeust:
                sb = kbd_system.sb_task_request;
                break;
            default: break;
            }
            if(sb) {
                write_shared_buffer(sb, time_us_64(), buf+1);
                bytes_left -= (1 + sb->size);
                buf += (1 + sb->size);
            } else {
                bytes_left = 0; // stop if encountered invalid
            }
        }
        *recv_size = 0;
        // send: + key_press, tb_motion(right only), task_response
        buf = send_buf + *send_size;
        buf[0] = comm_data_type_key_press;
        memcpy(buf+1, kbd_system.core1.key_press, hw_row_count);
        *send_size += (1 + hw_row_count);
        buf += (1 + hw_row_count);
        comm_data_type_t types[2] = { comm_data_type_tb_motion, comm_data_type_task_response };
        shared_buffer_t* sbs[2] = {
#ifdef KBD_NODE_RIGHT
            kbd_system.sb_tb_motion,
#else
            NULL,
#endif
            kbd_system.sb_task_response
        };
        static uint64_t ts[2] = {0,0}; // tb_motion, task_response
        for(uint8_t i=0; i<2; i++) {
            shared_buffer_t* sb = sbs[i];
            if(sb && sb->ts > ts[i]) {
                buf[0] = types[i];
                read_shared_buffer(sb, ts+i, buf+1);
                *send_size += (1 + sb->size);
                buf += (1 + sb->size);
            }
        }
    }

    // send
    udp_server_t* server = &kbd_system.core1.udp_server;
    udp_server_send(server, index, NULL, NULL, 0);
}

#endif

void core1_main() {
#ifdef KBD_NODE_AP
    cyw43_arch_enable_ap_mode(hw_ap_name, hw_ap_password, CYW43_AUTH_WPA2_AES_PSK);

    ip4_addr_t mask;
    IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);
    dhcp_server_init(&kbd_system.core1.dhcp_server, &kbd_system.core1.tcp_server->gw, &mask);
#else
    while(cyw43_arch_wifi_connect_timeout_ms(hw_ap_name, hw_ap_password, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        // keep trying to connect
    }
#endif

    tcp_server_open(&kbd_system.core1.tcp_server, KBD_NODE_NAME);

    udp_server_open(&kbd_system.core1.udp_server);

    kbd_system.ledB = kbd_led_state_BLINK_FAST;

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
    uint32_t ks_last_ms = board_millis();
    uint32_t comm_last_ms = board_millis();
#endif

    while(true) {
        led_task();

        if(kbd_system.core1.reboot) {
            pfb_perform_update();
        }

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
        // scan key presses, @ 5 ms
        do_if_elapsed(&ks_last_ms, 5, NULL, key_scan_task);
#endif

        // publish UDP, @ 5 ms
        do_if_elapsed(&comm_last_ms, 5, NULL, comm_task);

        cyw43_arch_poll(); // 1 ms loop for poll
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1));
    }
}
