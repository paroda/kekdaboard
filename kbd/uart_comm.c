#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/irq.h"

#include "uart_comm.h"

#define BAUD_RATE 1843200 // 921600 // 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

uart_comm_t* comms[1];

static void on_uart_rx_0(void) {
    peer_comm_on_receive(comms[0]->config);
}

static uint8_t get_0(void) {
    return uart_getc(comms[0]->uart);
}

static void put_0(uint8_t b) {
    uart_putc_raw(comms[0]->uart, b);
}

uart_comm_t* uart_comm_create(uart_inst_t* uart, uint8_t gpio_TX, uint8_t gpio_RX,
                              peer_comm_config_t* config) {
    uart_comm_t* comm = (uart_comm_t*) malloc(sizeof(uart_comm_t));
    comm->config = config;
    comm->uart = uart;
    comm->gpio_RX = gpio_RX;
    comm->gpio_TX = gpio_TX;

    // instance specific callback/handlers (TODO: make dynamic instead of fixed)
    comms[0] = comm;
    comm->on_rx = on_uart_rx_0;
    peer_comm_set_handlers(config, get_0, put_0, time_us_64);

    comm->baud_rate = uart_init(comm->uart, BAUD_RATE);

    gpio_set_function(comm->gpio_TX, GPIO_FUNC_UART);
    gpio_set_function(comm->gpio_RX, GPIO_FUNC_UART);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(comm->uart, false, false);

    // Set data format
    uart_set_format(comm->uart, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(comm->uart, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    comm->uart_irq = comm->uart == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(comm->uart_irq, comm->on_rx);
    irq_set_enabled(comm->uart_irq, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(comm->uart, true, false);

    return comm;
}

void uart_comm_free(uart_comm_t* comm) {
    irq_remove_handler(comm->uart_irq, comm->on_rx);
    irq_set_enabled(comm->uart_irq, false);
    free(comm);
}
