#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include "lwip/tcp.h"

typedef struct {
    struct tcp_pcb* pcb;
    ip_addr_t gw;
} tcp_server_t;

typedef struct {
    struct tcp_pcb* pcb;
    ip_addr_t* gw;
} tcp_client_t;


bool tcp_server_open(tcp_server_t* server, const char *server_name);

void tcp_server_close(tcp_server_t* server);

#endif
