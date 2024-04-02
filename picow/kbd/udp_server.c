#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwip/pbuf.h"

#include "data_model.h"

void udp_server_send(udp_server_t* server, u8_t index,
                     struct udp_pcb* pcb, const ip_addr_t* addr, u16_t port) {
    u8_t size = server->send_size[index];
    if(size>0) {
        struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, size+1, PBUF_RAM);
        u8_t* b = (u8_t*) p->payload;
        b[0] = index;
        memcpy(b+1, server->send_buf[index], size);
        err_t err = pcb ? udp_sendto(pcb, p, addr, port) : udp_send(server->pcb, p);
        if(err==ERR_OK) {
            server->send_size[index] = 0;
        }
        pbuf_free(p);
    }
}

static void udp_server_recv(void* arg, struct udp_pcb* pcb, struct pbuf* p,
                            const ip_addr_t* addr, u16_t port) {
    udp_server_t* server = (udp_server_t*) arg;
    u8_t index;
    if(!p) return;
    if(p->tot_len > 0) {
        index = ((u8_t*)p->payload)[0];
        server->recv_size[index] = p->tot_len-1;
        pbuf_copy_partial(p, server->recv_buf[index], p->tot_len-1, 1);
    }
    pbuf_free(p);
    udp_server_send(server, index, pcb, addr, port);
}

void udp_server_open(udp_server_t* server) {
    server->pcb = udp_new();

#ifdef KBD_NODE_AP
    udp_bind(server->pcb, IP_ADDR_ANY, hw_udp_port);
#else
    ip_addr_t addr = {
        .addr = KBD_AP_IP,
    };
    udp_connect(server->pcb, &addr, hw_udp_port);
#endif

    udp_recv(server->pcb, udp_server_recv, server);
}
