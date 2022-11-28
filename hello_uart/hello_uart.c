#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

static uint32_t blink_interval_ms = 100;

static volatile bool peer_ready = false;
static volatile bool peer_busy = false;
static uint32_t baud_rate = 0;

#define UART_ID uart0
#define BAUD_RATE 1843200 // 921600 // 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

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
    static bool led_state = false;
    static uint32_t start_ms = 0;
    if(start_ms == 0) start_ms = board_millis();

    // Blink every interval ms
    if (board_millis() - start_ms < blink_interval_ms)
        return; // not enough time
    start_ms += blink_interval_ms;

    board_led_write(led_state);

    led_state = 1 - led_state; // toggle
}

static volatile uint64_t start_us = 0u;
static volatile uint64_t end_us = 0u;
static volatile uint8_t last_byte = 0u;
static volatile uint32_t recv_count = 0u;
static volatile uint32_t sent_count = 0u;
static volatile uint32_t bad_count = 0u;

static volatile uint32_t BATCH = 10;

void print_stats(void) {
    uint64_t end_us = time_us_64();
    uint64_t dt = end_us - start_us;
    printf("\nFinished batch, recv: %u, sent: %u, bad: %u, time: %lu", recv_count, sent_count, bad_count, dt);
    start_us = 0u;
    last_byte = 0u;
    recv_count = 0u;
    sent_count = 0u;
    bad_count = 0u;
}

// RX interrupt handler
void on_uart_rx(void) {
    while(uart_is_readable(UART_ID)) {
        uint8_t c = uart_getc(UART_ID);
        if(c==0x80) {
            peer_ready = true;
            blink_interval_ms = 500;
            uart_putc_raw(UART_ID, 0x81);
        } else if(c==0x81) {
            peer_ready = true;
            blink_interval_ms = 500;
        } else if(c==0x82) {
            start_us = time_us_64();
            last_byte = 1u;
            uart_putc_raw(UART_ID, last_byte);
            sent_count++;
        } else if(c==0x83) {
            end_us = time_us_64();
            print_stats();
            peer_busy = false;
        } else {
            recv_count++;
            if(recv_count == BATCH) {
                end_us = time_us_64();
                uart_putc_raw(UART_ID, 0x83);
                print_stats();
                peer_busy = false;
                return;
            }
            if(last_byte++ == 99) last_byte = 0;
            if(last_byte != c) bad_count++;
            if(last_byte++ == 99) last_byte = 0;
            uart_putc_raw(UART_ID, last_byte);
            sent_count++;
        }
    }
}

void setup_uart(void) {
    // Set up our UART with a basic baud rate.
    /* uart_init(UART_ID, 2400); */
    /* uart_init(UART_ID, 9600); */
    baud_rate = uart_init(UART_ID, BAUD_RATE);

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
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

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

void test_task(void) {
    static bool inited = false;
    static bool last_state = false;

    if(!inited) {
        gpio_init(15);
        gpio_set_dir(15, GPIO_IN);
        inited = true;
    }

    bool state = gpio_get(15);
    if(state && !last_state) {
        printf("\nHi, starting batch..");
        BATCH *= 10;
        if(BATCH > 10000) BATCH = 10;
        peer_busy = true;
        start_us = time_us_64();
        uart_putc_raw(UART_ID, 0x82);
    }
    last_state = state;
}

int main(void) {
    stdio_init_all();
    printf("\nHello from PICO. Booting up..");

    sleep_ms(100);

    setup_uart();

    while(true) {
        sleep_ms(10);
        led_blinking_task();

        if(peer_ready && !peer_busy) {
            test_task();
        }

        if(!peer_ready) {
            uart_putc_raw(UART_ID, 0x80);
        }

        static uint32_t start_ms = 0;
        if(start_ms == 0) start_ms = board_millis();

        if(board_millis() - start_ms > 3000) {
            start_ms += 3000;
            printf("\nHello again from PICO. still beating.. ready: %d, busy: %d, baud: %d",
                   peer_ready, peer_busy, baud_rate);
        }
    }
}
