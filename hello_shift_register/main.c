/*
  74HC595 test
  .   ___   ___
  Q1  |  |_|  |   VCC
  Q2  |       |   Q0
  Q3  |       |   SER
  Q4  |       |   OE
  Q5  |       |   RCLK
  Q6  |       |   SRCLK
  Q7  |       |   SRCLR
  GND |       |   QH'
  .   ---------

  74HC595     pico
  -------     ----
  VCC         3.3V
  SER         GP12
  OE          GND
  RCLK        GP10
  SRCLK       GP11
  SRCLR       3.3V
*/

#include "pico/stdlib.h"
#include "shift_register.h"

// Define shift register pins
#define LATCH_PIN 13
#define CLOCK_PIN 14
#define DATA_PIN 15

#define BOARD_LED_PIN 25

#define LED_PIN 10
#define BTN_PIN 11

#define QPIN1 6
#define QPIN2 7
#define QPIN3 8
#define QPIN4 9

static void play(shift_register_t *sr) {
    shift_register_sync(sr);
    for (int i = 0; i < 8; i++) {
        sleep_ms(500);
        gpio_put(LED_PIN, true);
        sleep_ms(100);
        gpio_put(LED_PIN, false);
        sleep_ms(100);
        gpio_put(LED_PIN, sr->bits[i]);
        sleep_ms(100);
        gpio_put(LED_PIN, false);
    }
}

int main() {
    gpio_init(LATCH_PIN);
    gpio_set_dir(LATCH_PIN, GPIO_OUT);
    gpio_init(CLOCK_PIN);
    gpio_set_dir(CLOCK_PIN, GPIO_OUT);
    gpio_init(BOARD_LED_PIN);
    gpio_set_dir(BOARD_LED_PIN, GPIO_OUT);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(DATA_PIN);
    gpio_set_dir(DATA_PIN, GPIO_IN);
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);

    shift_register_t *sr = shift_register_new(SHIFT_REGISTER_MODE_PISO, 8, LATCH_PIN, CLOCK_PIN, DATA_PIN);
    shift_register_sync(sr);

    gpio_put(LED_PIN, false);

    while (1) {
        static bool led_state = false;

        sleep_ms(100);
        gpio_put(BOARD_LED_PIN, led_state);
        led_state = !led_state;

        if (gpio_get(BTN_PIN)) play(sr);
    }
}
