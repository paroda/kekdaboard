#ifndef _UART_COMM_H
#define _UART_COMM_H

#include <stdint.h>
#include <stdbool.h>

#include "hardware/uart.h"

#include "peer_comm.h"

typedef struct {
    peer_comm_config_t* config;

    uart_inst_t* uart;
    uint8_t gpio_TX;
    uint8_t gpio_RX;

    int uart_irq;

    uint32_t baud_rate;

    void (*on_rx)(void);
} uart_comm_t;

uart_comm_t* uart_comm_create(uart_inst_t* uart, uint8_t gpio_TX, uint8_t gpio_RX,
                              peer_comm_config_t* config);

void uart_comm_free(uart_comm_t* comm);


#endif
