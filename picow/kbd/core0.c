#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/cyw43_arch.h"

#include "hw_model.h"
#include "data_model.h"

#ifdef KBD_NODE_AP
#include "usb_hid.h"
#endif

static void toggle_led(void* param) {
    kbd_led_t* led = (kbd_led_t*) param;
    led->on = !led->on;
    if(led->wl_led)
        cyw43_arch_gpio_put(led->gpio, led->on);
    else
        gpio_put(led->gpio, led->on);
}

static int32_t led_millis_to_toggle(kbd_led_t* led, kbd_led_state_t state) {
    // return milliseconds after which to toggle the led
    // return -1 to indicate skip toggle
    switch(state) {
    case kbd_led_state_OFF:
        return led->on ? 0 : -1;
    case kbd_led_state_ON:
        return led->on ? -1 : 0;
    case kbd_led_state_BLINK_LOW:    // 100(on), 900(off)
        return led->on ? 100 : 900;
    case kbd_led_state_BLINK_HIGH:   // 900, 100
        return led->on ? 900 : 100;
    case kbd_led_state_BLINK_SLOW:   // 1000, 1000
        return led->on ? 2000 : 2000;
    case kbd_led_state_BLINK_NORMAL: // 500, 500
        return led->on ? 500 : 500;
    case kbd_led_state_BLINK_FAST:   // 100, 100
        return led->on ? 50 : 50;
    }
    return -1;
}

#ifdef KBD_NODE_AP
static void update_loop_count(void* param) {
    static uint32_t loop_count = 0;
    static uint32_t loop_time = 0;
    uint32_t t = board_millis()/10000;
    if(loop_time==t) {
        loop_count++;
    } else {
        set_core0_debug(0, loop_count/10);
        loop_count = 1;
        loop_time = t;
    }
}
#endif

static void led_task() {

#ifdef KBD_NODE_AP
    static uint32_t ms=0;
    do_if_elapsed(&ms, 50, NULL, update_loop_count);

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

static void update_comm_state(volatile kbd_comm_state_t* comm_state,
                              uint8_t* recv_buf, uint8_t* recv_size,
                              uint8_t* send_buf, uint8_t* send_size) {
    /* printf("comm_state %d, recv_size %d, recv %d\n", *comm_state, *recv_size, recv_buf[0]); */
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
        *send_size = 1;
        send_buf[0] = *comm_state;
    } else {
        *send_size = 0;
    }
}

typedef enum {
    comm_data_type_system_state = 0,
    comm_data_type_key_press,
    comm_data_type_tb_motion,
    comm_data_type_task_request,
    comm_data_type_task_response,
    comm_data_type_task_id,
} comm_data_type_t;

#ifdef KBD_NODE_AP

static void comm_task(void* param) {
    // debug counter
    static uint32_t ct_count_time = 0;
    static uint32_t ct_count = 0;
    uint32_t ct_t = board_millis()/10000;
    if(ct_count_time==ct_t) {
        ct_count++;
    } else {
        set_core0_debug(1, ct_count/10);
        ct_count = 1;
        ct_count_time = ct_t;
    }
    // comm task
    uint8_t index = *((uint8_t*) param);
    volatile kbd_comm_state_t* comm_state = kbd_system.comm_state+index;
    udp_server_t* server = &kbd_system.core0.udp_server;
    uint8_t* recv_buf = server->recv_buf[index];
    uint8_t* recv_size = server->recv_size+index;
    uint8_t* send_buf = server->send_buf[index];
    uint8_t* send_size = server->send_size+index;
    if(*recv_size>0) {
        printf("Received UDP: size: %d, state: %d\n", *recv_size, recv_buf[0]);
        update_comm_state(comm_state, recv_buf, recv_size, send_buf, send_size);
        static uint8_t ack_req_id[2] = {0,0}; // last request acknowledged
        static uint8_t rcv_res_id[2] = {0,0}; // last response received
        if(*comm_state==kbd_comm_state_data) {
            // recv: + key_press, tb_motion(right only), task_response
            int bytes_left = *recv_size - 1;
            uint8_t* buf = recv_buf+1;
            while(bytes_left>0) {
                shared_buffer_t* sb = NULL;
                switch(buf[0]) {
                case comm_data_type_key_press:
                    sb = index==0 ? kbd_system.sb_left_key_press : kbd_system.sb_right_key_press;
                    break;
                case comm_data_type_tb_motion:
                    sb = kbd_system.sb_tb_motion;
                    break;
                case comm_data_type_task_response:
                    sb = index==0 ? kbd_system.sb_left_task_response : kbd_system.sb_right_task_response;
                    rcv_res_id[index] = buf[1];
                    break;
                case comm_data_type_task_id:
                    ack_req_id[index] = buf[1];
                default: break;
                }
                if(sb) {
                    write_shared_buffer(sb, time_us_64(), buf+1);
                    bytes_left -= (1 + sb->size);
                    buf += (1 + sb->size);
                } else if(buf[0]==comm_data_type_task_id) {
                    bytes_left -= 2;
                    buf += 2;
                } else {
                    bytes_left = 0; // stop if encountered invalid
                }
            }
            *recv_size = 0; // mark recv done
            // package data to send
            buf = send_buf + *send_size;
            // add system state
            static uint64_t state_ts = 0;
            buf[0] = comm_data_type_system_state;
            read_shared_buffer(kbd_system.sb_state, &state_ts, buf+1);
            *send_size += (1 + kbd_system.sb_state->size);
            buf += (1 + kbd_system.sb_state->size);
            // add request
            static uint64_t req_ts[2] = {0,0};
            shared_buffer_t* sb = index==0 ? kbd_system.sb_left_task_request : kbd_system.sb_right_task_request;
            read_shared_buffer(sb, req_ts+index, buf+1);
            if(buf[1] != ack_req_id[index]) {
                buf[0] = comm_data_type_task_request;
                *send_size += (1 + sb->size);
                buf += (1 + sb->size);
            }
            // acknowledge resposne received
            buf[0] = comm_data_type_task_id;
            buf[1] = rcv_res_id[index];
            *send_size += 2;
            buf += 2;
        } else {
            *recv_size = 0;
        }
    }
}

void wifi_poll(void* param) {
    static uint32_t poll_max_ms = 0;
    static uint32_t poll_ms = 0;
    static uint32_t poll_count = 0;
    static uint32_t poll_time = 0;
    uint32_t t0 = board_millis();
    cyw43_arch_poll(); // do the work
    uint32_t t1 = board_millis();
    uint32_t dt = t1 - t0;
    uint32_t t = t1/10000;
    if(poll_time == t) {
        if(dt>poll_max_ms) poll_max_ms=dt;
        poll_ms += dt;
        poll_count++;
    } else {
        set_core0_debug(2, poll_max_ms);
        set_core0_debug(3, poll_ms/(poll_count>0?poll_count:1));
        set_core0_debug(4, poll_count/10);
        poll_max_ms = 0;
        poll_ms = 0;
        poll_count = 1;
        poll_time = t;
    }
}

#else

static void rssi_task(void* param) {
    (void)param;
    int32_t rssi;
    // command: 254 WLC_GET_RSSI
    cyw43_ioctl(&cyw43_state, 254, sizeof(int32_t), (uint8_t*) &rssi, CYW43_ITF_STA);
    kbd_system.wifi_rssi = rssi;
}

static void key_scan_task(void* param) {
    (void)param;
    // scan key presses with kbd_hw.ks and save to core0.key_press
    key_scan_update(kbd_hw.ks);
    memcpy(kbd_system.core0.key_press, kbd_hw.ks->keys, hw_row_count);
}

static void comm_task(void* param) {
    (void) param;
#ifdef KBD_NODE_LEFT
    uint8_t index = 0;
#else
    uint8_t index = 1;
#endif
    volatile kbd_comm_state_t* comm_state = kbd_system.comm_state+index;
    udp_server_t* server = &kbd_system.core0.udp_server;
    uint8_t* recv_buf = server->recv_buf[index];
    uint8_t* recv_size = server->recv_size+index;
    uint8_t* send_buf = server->send_buf[index];
    uint8_t* send_size = server->send_size+index;
    if(*recv_size>0) printf("Received UDP: size: %d, state: %d\n", *recv_size, recv_buf[0]);
    update_comm_state(comm_state, recv_buf, recv_size, send_buf, send_size);
    static uint8_t rcv_req_id = 0; // last request received
    static uint8_t ack_res_id = 0; // last response acknowledged
    if(*comm_state==kbd_comm_state_data) {
        // recv: + system_state, task_request
        int bytes_left = *recv_size - 1;
        uint8_t* buf = recv_buf+1;
        while(bytes_left>0) {
            shared_buffer_t* sb = NULL;
            switch(buf[0]) {
            case comm_data_type_system_state:
                sb = kbd_system.sb_state;
                break;
            case comm_data_type_task_request:
                sb = kbd_system.sb_task_request;
                rcv_req_id = buf[1];
                break;
            case comm_data_type_task_id:
                ack_res_id = buf[1];
            default: break;
            }
            if(sb) {
                write_shared_buffer(sb, time_us_64(), buf+1);
                bytes_left -= (1 + sb->size);
                buf += (1 + sb->size);
            } else if(buf[0]==comm_data_type_task_id) {
                bytes_left -= 2;
                buf += 2;
            } else {
                bytes_left = 0; // stop if encountered invalid
            }
        }
        *recv_size = 0; // mark recv done
        // package data to send
        buf = send_buf + *send_size;
        // add key_press
        buf[0] = comm_data_type_key_press;
        memcpy(buf+1, kbd_system.core0.key_press, hw_row_count);
        *send_size += (1 + hw_row_count);
        buf += (1 + hw_row_count);
#ifdef KBD_NODE_RIGHT
        // add tb_motion
        static uint64_t tbm_ts = 0;
        if(kbd_system.sb_tb_motion->ts > tbm_ts) {
            buf[0] = comm_data_type_tb_motion;
            read_shared_buffer(kbd_system.sb_tb_motion, &tbm_ts, buf+1);
            *send_size += (1 + kbd_system.sb_tb_motion->size);
            buf += (1 + kbd_system.sb_tb_motion->size);
        }
#endif
        // add response
        static uint64_t res_ts = 0;
        read_shared_buffer(kbd_system.sb_task_response, &res_ts, buf+1);
        if(buf[1] != ack_res_id) {
            buf[0] = comm_data_type_task_response;
            *send_size += (1 + kbd_system.sb_task_response->size);
            buf += (1 + kbd_system.sb_task_response->size);
        }
        // acknowledge request received
        buf[0] = comm_data_type_task_id;
        buf[1] = rcv_req_id;
        *send_size += 2;
        buf += 2;
    } else {
        *recv_size = 0;
    }
    /* printf("sending comm_state %d, send_size %d, send %d \n", *comm_state, *send_size, send_buf[0]); */
    // send
    udp_server_send(server, index, NULL, NULL, 0);
}

void wifi_poll(void* param) {
    cyw43_arch_poll();
}

#endif

void core0_main() {
#ifdef KBD_NODE_AP
    cyw43_arch_enable_ap_mode(hw_ap_name, hw_ap_password, CYW43_AUTH_WPA2_AES_PSK);
    printf("WiFi started %s, %s", hw_ap_name, hw_ap_password);
#else
    cyw43_arch_enable_sta_mode();

    while(cyw43_arch_wifi_connect_timeout_ms(hw_ap_name, hw_ap_password, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        // keep trying to connect
        sleep_ms(1000);
        printf("Retrying to connect to WiFi from %s, %x\n", KBD_NODE_NAME, KBD_NODE_IP);
    }
    printf("Connected to WiFi\n");
#endif

    tcp_server_open(&kbd_system.core0.tcp_server, KBD_NODE_NAME);

    uint32_t ts = board_millis();
    uint32_t poll_last_ts = ts;

#ifdef KBD_NODE_AP
    udp_server_open(&kbd_system.core0.udp_server, comm_task);
#else
    udp_server_open(&kbd_system.core0.udp_server, NULL);

    uint32_t rssi_last_ts = ts;
    uint32_t ks_last_ms = ts;
    uint32_t comm_last_ms = ts+2;
#endif

    while(true) {
        if(!kbd_system.firmware_downloading)
        {
            /* sleep_ms(1000); */

            led_task();

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
            // get current wifi strength
            do_if_elapsed(&rssi_last_ts, 1000, NULL, rssi_task);

            // scan key presses, @ 5 ms
            do_if_elapsed(&ks_last_ms, 5, NULL, key_scan_task);

            // publish UDP, @ 5 ms
            do_if_elapsed(&comm_last_ms, 10, NULL, comm_task);
#endif
        }

        // AP need to handle UDP packets from both left and right
        // Give it double frequency as the left/right nodes
#ifdef KBD_NODE_AP
        do_if_elapsed(&poll_last_ts, 1, NULL, wifi_poll);
#else
        do_if_elapsed(&poll_last_ts, 2, NULL, wifi_poll);
#endif
    }
}
