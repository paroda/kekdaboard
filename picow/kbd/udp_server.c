#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"

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
        printf("Sending UDP size: %d, to %d, state: %d\n", size, index, b[1]);
        err_t err = pcb ?
            udp_sendto(pcb, p, addr, port) : // ACCESS_POINT
            udp_send(server->pcb, p);        // LEFT/RIGHT
        if(err==ERR_OK) {
            server->send_size[index] = 0;
        } else {
            printf("Failed to send UDP\n");
        }
        pbuf_free(p);
    }
}

void udp_server_recv(void* arg, struct udp_pcb* pcb, struct pbuf* p,
                     const ip_addr_t* addr, u16_t port) {
    udp_server_t* server = (udp_server_t*) arg;
    u8_t index;
    printf("Received UDP\n");
    if(!p) return;
    if(p->tot_len > 0) {
        index = ((u8_t*)p->payload)[0];
        server->recv_size[index] = p->tot_len-1;
        pbuf_copy_partial(p, server->recv_buf[index], p->tot_len-1, 1);
    }
    pbuf_free(p);
    if(server->comm_task) {
        server->comm_task(&index);
        udp_server_send(server, index, pcb, addr, port);
    }
}

bool udp_server_open(udp_server_t* server, void (*comm_task)(void*)) {
    server->pcb = udp_new();
    server->comm_task = comm_task;

#ifdef KBD_NODE_AP
    if(udp_bind(server->pcb, IP_ADDR_ANY, hw_udp_port) != ERR_OK) {
        printf("Failed to bind to UDP port %d\n", hw_udp_port);
        return false;
    }
#else
    ip_addr_t addr = {.addr = KBD_AP_IP};
    if(udp_connect(server->pcb, &addr, hw_udp_port) != ERR_OK) {
        printf("Failed to connect UDP to %x %d\n", KBD_AP_IP, hw_udp_port);
        return false;
    }
#endif

    udp_recv(server->pcb, udp_server_recv, server);
    return true;
}
