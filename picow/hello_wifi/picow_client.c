/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"

#define WIFI_SSID "picow_test"
#define WIFI_PASSWORD "password"

#define TCP_SERVER_IP "192.168.4.1"
#define TCP_PORT 80

#define UDP_PORT 81
#define BEACON_MSG_LEN_MAX 32
#define BEACON_INTERVAL_US 2000000

#define DEBUG_printf printf
#define BUF_SIZE 2048

#define POLL_TIME_S 5

typedef struct TCP_CLIENT_T_ {
    struct tcp_pcb *tcp_pcb;
    ip_addr_t remote_addr;
    uint8_t buffer_out[BUF_SIZE];
    int sent_len;
    uint8_t buffer_in[BUF_SIZE];
    int recv_len;
    int test_len;
    uint64_t start_us;
    bool complete;
    bool connected;
} TCP_CLIENT_T;

uint32_t board_millis() {
    return us_to_ms(time_us_64());
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    DEBUG_printf("sent %u\n", len);
    state->sent_len += len;
    if (state->sent_len >= state->test_len) {
        uint64_t dt = time_us_64() - state->start_us;
        DEBUG_printf("Sent complete: t = %llu\n", dt);
    }
    return ERR_OK;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    if (err != ERR_OK) {
        printf("Failed to connect to server:  %d\n", err);
        return err;
    }
    state->connected = true;
    DEBUG_printf("Connected to server\n");
    return ERR_OK;
}

static void run_tcp_test(TCP_CLIENT_T* state) {
    if(state->connected && state->complete) {
        DEBUG_printf("\n\n\n\nStarting test..\n");
        switch(state->test_len) {
        case 1:
            state->test_len = 10;
            break;
        case 10:
            state->test_len = 50;
            break;
        case 50:
            state->test_len = 100;
            break;
        case 100:
            state->test_len = 500;
            break;
        case 500:
            state->test_len = 1000;
            break;
        case 1000:
            state->test_len = 2000;
            break;
        default:
            state->test_len = 1;
            break;
        }
        state->start_us = time_us_64();
        state->sent_len = 0;
        state->recv_len = 0;
        state->complete = false;
        err_t err = tcp_write(state->tcp_pcb, state->buffer_out, state->test_len, TCP_WRITE_FLAG_COPY);
        if(err != ERR_OK) {
            state->complete = true;
        }
    } else {
        DEBUG_printf("\n\n\nSkipped test as busy!\n\n\n");
    }
}

static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    // TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    // DEBUG_printf("tcp_client_poll\n");
    // run test
    // run_tcp_test(state);
    return ERR_OK;
}

static void tcp_client_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err %d\n", err);
    }
}

err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    if (!p) {
        DEBUG_printf("aborted test..\n");
        state->complete = true;
        return -1;
    }
    if (p->tot_len > 0) {
        DEBUG_printf("recv %d err %d\n", p->tot_len, err);
        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - state->recv_len;
        state->recv_len += pbuf_copy_partial(p, state->buffer_in + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);
    // If we have received the whole buffer, test complete
    if (state->recv_len >= state->test_len) {
        uint64_t dt = time_us_64() - state->start_us;
        DEBUG_printf("Receive complete: t = %llu\n", dt);
        DEBUG_printf("Finished test\n");
        state->complete = true;
    }
    return ERR_OK;
}

static bool tcp_client_open(void *arg) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    DEBUG_printf("Connecting to %s port %u\n", ip4addr_ntoa(&state->remote_addr), TCP_PORT);
    state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
    if (!state->tcp_pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    tcp_arg(state->tcp_pcb, state);
    tcp_poll(state->tcp_pcb, tcp_client_poll, POLL_TIME_S * 2);
    tcp_sent(state->tcp_pcb, tcp_client_sent);
    tcp_recv(state->tcp_pcb, tcp_client_recv);
    tcp_err(state->tcp_pcb, tcp_client_err);

    err_t err = tcp_connect(state->tcp_pcb, &state->remote_addr, TCP_PORT, tcp_client_connected);

    return err == ERR_OK;
}

// Perform initialisation
static TCP_CLIENT_T* tcp_client_init(void) {
    TCP_CLIENT_T *state = calloc(1, sizeof(TCP_CLIENT_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    state->test_len = 1;
    state->complete = true;
    ip4addr_aton(TCP_SERVER_IP, &state->remote_addr);
    return state;
}

///// UDP

typedef struct UDP_STATE_T_ {
    struct udp_pcb* pcb;
    uint64_t start;
} UDP_STATE_T;

void udp_recv_callback(void *arg, struct udp_pcb* pcb, struct pbuf* p,
                       const ip_addr_t* addr, u16_t port) {
    UDP_STATE_T* state = (UDP_STATE_T*) arg;
    uint64_t dt = time_us_64() - state->start;
    DEBUG_printf("Received UDP len: %d, time: %llu\n", p->tot_len, dt);
    state->start = 0;
    pbuf_free(p);
}

UDP_STATE_T* udp_state_init() {
    UDP_STATE_T* state = calloc(1, sizeof(UDP_STATE_T));
    if(!state) {
        DEBUG_printf("failed to allocate UDP state\n");
        return NULL;
    }
    state->start = 0;
    return state;
}

void test_send_udp(UDP_STATE_T* state) {
    static uint64_t t = 0;
    if(t==0) t = time_us_64();
    if(BEACON_INTERVAL_US <= (time_us_64()-t)) {
        t += BEACON_INTERVAL_US;
        if(state->start == 0) {
            static int counter = 0;
            struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, BEACON_MSG_LEN_MAX, PBUF_RAM);
            char *req = (char*)p->payload;
            memset(req, 1, BEACON_MSG_LEN_MAX);
            state->start = time_us_64();
            err_t err = udp_send(state->pcb, p);
            pbuf_free(p);
            if(err!=ERR_OK) {
                DEBUG_printf("Failed to send UDP packet! error=%d", err);
            } else {
                DEBUG_printf("\n\n\nSent packet %d\n", counter);
                counter++;
            }
        } else {
            DEBUG_printf("Skipped UDB send since busy!\n");
        }
    }
}

UDP_STATE_T* init_udp_test() {
    UDP_STATE_T* state = udp_state_init();
    state->pcb = udp_new();
    ip_addr_t addr;
    ip4addr_aton(TCP_SERVER_IP, &addr);
    udp_connect(state->pcb, &addr, UDP_PORT);
    udp_recv(state->pcb, udp_recv_callback, state);
    return state;
}

void run_test(void) {
    TCP_CLIENT_T* tcp_state = tcp_client_init();
    tcp_client_open(tcp_state);

    UDP_STATE_T* udp_state = init_udp_test();

    while(true) {
        test_send_udp(udp_state);
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }

    free(tcp_state);
    free(udp_state);
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        DEBUG_printf("failed to initialise\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect to WiFi.\n");
        return 1;
    } else {
        printf("Connected to WiFi.\n");
    }
    run_test();
    cyw43_arch_deinit();
    return 0;
}
