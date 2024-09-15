#include <stdio.h>

#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

int btstack_main(void);

int btstack_process(void);

void do_poll();

#define POLL_INTERVAL_MS 5
static void poll_handler(struct btstack_timer_source *ts) {
  do_poll();
  btstack_run_loop_set_timer(ts, POLL_INTERVAL_MS);
  btstack_run_loop_add_timer(ts);
  sleep_ms(3);
}

static btstack_timer_source_t poll;

int main() {

  stdio_init_all();
  sleep_ms(2000);

  printf("Booting up..\n");

  // initialize CYW43 driver architecture (will enable BT if CYW43_ENABLE_BLUETOOTH == 1)
  if (cyw43_arch_init()) {
    printf("failed to initialise cyw43_arch\n");
    printf("abort!\n");
    sleep_ms(2000);
  }

  printf("cyw43 inited\n");

  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

  btstack_main();

  // btstack_run_loop_execute is only required when using the 'polling' method
  // (e.g. using pico_cyw43_arch_poll library). btstack_run_loop_execute();

  poll.process = &poll_handler;
  btstack_run_loop_set_timer(&poll, POLL_INTERVAL_MS);
  btstack_run_loop_add_timer(&poll);

  btstack_run_loop_execute();

  return 0;
}

void do_poll() {
  static uint32_t ts_led = 0;
  if (ts_led == 0)
    ts_led = btstack_run_loop_get_time_ms();

  static uint8_t led = 1;

  uint32_t ts = btstack_run_loop_get_time_ms();
  if (ts > ts_led + 300) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led);
    led = !led;
    ts_led = ts;
  }
  btstack_process();
}
