/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

static uint32_t blink_interval_ms = 300;
static void led_blinking_task(void);
static void serial_log_task(void);

#define DATA_SIZE 8
static volatile uint8_t my_data[DATA_SIZE] = {0};
static volatile uint32_t my_data_pos = 0;
static bool my_data_sent = true;
static volatile uint8_t peer_data[DATA_SIZE] = {0};
static volatile uint32_t peer_data_pos = 0;
static bool peer_data_received = false;

static volatile bool found_uart_peer = false;

#define MY_CTRL_MASK 0x80

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

static int chars_rxed = 0;


static bool led_A = false;
static bool led_B = false;

void update_leds(void) {
    bool btn = gpio_get(18);

    if(btn) {
        led_A = gpio_get(15);
        led_B = gpio_get(14);
        if(my_data_sent) {
            my_data[1] = led_A;
            my_data[2] = led_B;
            my_data_sent = false;
        }
    } else {
        if(peer_data_received) {
            my_data[1] = led_A = peer_data[1];
            my_data[2] = led_B = peer_data[2];
            peer_data_received = false;
        }
    }

    gpio_put(16, led_A);
    gpio_put(17, led_B);
}

/* void send_init_data(void) { */
/*     for(int i=0; i<DATA_SIZE; i++) { */
/*         my_data[i] = 0; */
/*     } */
/*     my_data[DATA_SIZE-1]=1; */
/*     send_my_data(); */
/* } */

void ping_uart_peer(void) {
    uart_putc_raw(UART_ID, 0x80);
}

static uint32_t receive_counter = 0;

// RX interrupt handler
void on_uart_rx(void) {
    printf("\nReceiving..");
    while(uart_is_readable(UART_ID)) {
        uint8_t c = uart_getc(UART_ID);
        if(c==0x80) {
            printf("\nReceived ping 0x80");
            found_uart_peer = true;
            uart_putc_raw(UART_ID, 0x81);
        } else if(c==0x81) {
            printf("\nReceived ping 0x81");
            found_uart_peer = true;
        } else if(c==0x82) {
            printf("\nReceived reset buffer pos 0x82");
            peer_data_pos = 0;
            peer_data_received = false;
            uart_putc_raw(UART_ID, 0x83);
            receive_counter++;
        } else if(c==0x83) {
            if(!my_data_sent) {
                printf("\nSending next byte 0x83");
                uart_putc_raw(UART_ID, my_data[my_data_pos++]);
                my_data_pos %= DATA_SIZE;
                if(my_data_pos==0) my_data_sent = true;
            }
        } else {
            printf("\nReceived byte [%d] %d", peer_data_pos, c);
            peer_data[peer_data_pos++] = c;
            peer_data_pos %= DATA_SIZE;
            if(peer_data_pos==0) peer_data_received = true;
            else uart_putc_raw(UART_ID, 0x83);
        }
    }

    /* static uint8_t i = 0; */
    /* while(uart_is_readable(UART_ID)) { */
    /*     peer_data[i] = uart_getc(UART_ID); */
    /*     printf("\nReceived peer_data[%d]=%d", i, peer_data[i]); */
    /*     /\* peer_data[1] = uart_getc(UART_ID); *\/ */
    /*     /\* printf("\nReceived peer_data[1]=%d", peer_data[1]); *\/ */
    /*     /\* peer_data[2] = uart_getc(UART_ID); *\/ */
    /*     /\* printf("\nReceived peer_data[2]=%d", peer_data[2]); *\/ */
    /*     if(!(++i < DATA_SIZE)) i=0; */
    /* } */

}

static uint32_t send_counter = 0;

void send_my_data(void) {
    if(!my_data_sent) {
        my_data_pos = 0;
        uart_putc_raw(UART_ID, 0x82);
        send_counter++;
    }

    /* printf("\nSending data.."); */
    /* uart_putc_raw(UART_ID, 0x82); */
    /* for(uint8_t i=0; i<DATA_SIZE; i++) { */
    /*     while(!uart_is_writable(UART_ID)) tight_loop_contents(); */
    /*     uart_putc_raw(UART_ID, my_data[i]); */
    /* } */
    /* printf("\nSent %d bytes..", DATA_SIZE); */

    /* uart_write_blocking(UART_ID, my_data, DATA_SIZE); */
    /* uart_tx_wait_blocking(UART_ID); */
    /* printf("\nSent %d bytes: ", DATA_SIZE); */
    /* for(uint8_t i=0; i<DATA_SIZE; i++) { */
    /*     printf("%d, ", my_data[i]); */
    /* } */

    /* for(uint8_t i = 0; i < DATA_SIZE ; i++) { */
    /*     uart_putc_raw(UART_ID, my_data[i]); */
    /*     printf("\nSent my_data[i]=%d", i, my_data[i]); */
    /* } */
    /* /\* uart_putc_raw(UART_ID, my_data[1]); *\/ */
    /* /\* printf("\nSent my_data[1]=%d", my_data[1]); *\/ */
    /* /\* uart_putc_raw(UART_ID, my_data[2]); *\/ */
    /* /\* printf("\nSent my_data[2]=%d", my_data[2]); *\/ */

    /* my_data[0] = 0; */
}

void setup_uart(void) {
    // Set up our UART with a basic baud rate.
    /* uart_init(UART_ID, 2400); */
    /* uart_init(UART_ID, 9600); */
    uart_init(UART_ID, BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    /* int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE); */

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    /* uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY); */

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, true);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
}

int main(void) {
    stdio_init_all();
    printf("\nHello from PICO. Booting up..");

    gpio_init(18);
    gpio_set_dir(18, GPIO_IN);

    gpio_init(14);
    gpio_set_dir(14, GPIO_IN);

    gpio_init(15);
    gpio_set_dir(15, GPIO_IN);

    gpio_init(16);
    gpio_set_dir(16, GPIO_OUT);

    gpio_init(17);
    gpio_set_dir(17, GPIO_OUT);

    sleep_ms(100);

    setup_uart();

    while(true) {
        sleep_ms(4);
        serial_log_task();
        led_blinking_task();

        if(!found_uart_peer) {
            ping_uart_peer();
        } else {
            send_my_data();
        }

        update_leds();
    }
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+

static uint32_t board_millis(void) {
    return to_ms_since_boot(get_absolute_time());
}

static void board_led_write(bool state) {
    static bool inited = false;

    if(!inited) {
        gpio_init(25);
        gpio_set_dir(25, GPIO_OUT);
        inited = true;
    }

    gpio_put(25, state);
}

static void led_blinking_task(void) {
    static uint32_t start_ms = 0;
    static bool led_state = false;

    // Blink every interval ms
    if (board_millis() - start_ms < blink_interval_ms)
        return; // not enough time
    start_ms += blink_interval_ms;

    board_led_write(led_state);

    led_state = 1 - led_state; // toggle
}

static void serial_log_task(void) {
    static uint32_t start_ms = 0;

    if(board_millis() - start_ms < 1000) return;

    start_ms += 1000;
    printf("\nHello again from PICO. still beating.. send/receive: %d/%d",
           send_counter, receive_counter);
}
