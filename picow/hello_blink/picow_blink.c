#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

typedef struct {
    int a;
    int b;
} item_t;

typedef struct {
    item_t i;
    item_t j;
    int x;
} object_t;

object_t o = {
    .x = 3,

    .i = {
        .a = 4
    }
};

int main() {
    stdio_init_all();
    if(cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }
    while(true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(100);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(100);
    }
}
