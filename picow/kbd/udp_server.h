#ifndef _UDP_SERVER_H
#define _UDP_SERVER_H

#include "lwip/udp.h"

#define UDP_BUF_MAX_SIZE 64

typedef struct {
    struct udp_pcb* pcb;
    // use 2 sets of buffers, 0-left, 1-right
    u8_t recv_buf[2][UDP_BUF_MAX_SIZE];
    u8_t recv_size[2];
    u8_t send_buf[2][UDP_BUF_MAX_SIZE];
    u8_t send_size[2];
} udp_server_t;

// when pcb is NULL, it will send to server->pcb
void udp_server_send(udp_server_t* server, u8_t index,
                     struct udp_pcb* pcb, const ip_addr_t* addr, u16_t port);

bool udp_server_open(udp_server_t* server);

#endif
