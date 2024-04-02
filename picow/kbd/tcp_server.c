#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwip/pbuf.h"

#include "pico_fota_bootloader.h"

#include "data_model.h"

static err_t tcp_close_client_connection(tcp_client_t* client, struct tcp_pcb* client_pcb, err_t close_err) {
    if (client_pcb) {
        assert(client && client->pcb == client_pcb);
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);
        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK) {
            tcp_abort(client_pcb);
            close_err = ERR_ABRT;
        }
        if (client) {
            free(client);
        }
    }
    return close_err;
}

static err_t tcp_server_sent(void* arg, struct tcp_pcb* pcb, u16_t len) {
    (void) arg;
    (void) pcb;
    (void) len;
    return ERR_OK;
}

static err_t tcp_server_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err) {
    (void) err;
    tcp_client_t* client = (tcp_client_t*) arg;
    if (!p) {
        return tcp_close_client_connection(client, pcb, ERR_OK);
    }
    assert(client && client->pcb == pcb);
    if (p->tot_len > 0) {
        /* DEBUG_printf("tcp_server_recv %d err %d\n", p->tot_len, err); */
        u8_t* b = (u8_t*)p->payload;
        if(b[0]==0x01) { // initialize flash
            /* DEBUG_printf("Initiating flash\n"); */
            pfb_initialize_download_slot();
        } else if(b[0]==0x02) { // flash
            u32_t offset = 1024 * (b[1]*0x100 + b[2]);
            u8_t buf[1024];
            pbuf_copy_partial(p, buf, 1024, 4);
            pfb_write_to_flash_aligned_256_bytes(buf, offset, 1024);
            /* DEBUG_printf("Flashed offset %u\n", offset); */
        } else if(b[0]==0x03) { // finalize flash
            /* DEBUG_printf("Flash finished\n"); */
            pfb_mark_download_slot_as_valid();
            kbd_system.core1.reboot = true;
        }
        tcp_recved(pcb, p->tot_len);
        u8_t res[1] = {0x00};
        tcp_write(pcb,res,1,TCP_WRITE_FLAG_COPY);
    }
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_server_poll(void* arg, struct tcp_pcb* pcb) {
    (void) arg;
    (void) pcb;
    return ERR_OK;
}

static void tcp_server_err(void* arg, err_t err) {
    tcp_client_t* client = (tcp_client_t*) arg;
    if (err != ERR_ABRT) {
        tcp_close_client_connection(client, client->pcb, err);
    }
}

static err_t tcp_server_accept(void* arg, struct tcp_pcb* client_pcb, err_t err) {
    if(err!=ERR_OK || client_pcb==NULL) return ERR_VAL;

    tcp_server_t* server = (tcp_server_t*) arg;
    tcp_client_t* client = (tcp_client_t*) malloc(sizeof(tcp_server_t));
    client->pcb = client_pcb;
    client->gw = &server->gw;

    // setup connection to client
    tcp_arg(client_pcb, client);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, hw_tcp_poll_time_s * 2);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}

bool tcp_server_open(tcp_server_t* server, const char *server_name) {
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    err_t err = tcp_bind(pcb, IP_ANY_TYPE, hw_tcp_port);
    if(err==ERR_OK) {
        server->pcb = tcp_listen_with_backlog(pcb, 1);
        tcp_arg(server->pcb, server);
        tcp_accept(server->pcb, tcp_server_accept);
        return true;
    }
    return false;
}

void tcp_server_close(tcp_server_t* server) {
    if (server->pcb) {
        tcp_arg(server->pcb, NULL);
        tcp_close(server->pcb);
        server->pcb = NULL;
    }
}
