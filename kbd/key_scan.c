#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "key_scan.h"

key_scan_t* key_scan_create(uint8_t row_count, uint8_t col_count,
                            uint8_t* gpio_rows, uint8_t* gpio_cols) {
    key_scan_t* ks = (key_scan_t*) malloc(sizeof(key_scan_t));
    ks->row_count = row_count;
    ks->col_count = col_count;
    ks->gpio_rows = (uint8_t*) malloc(row_count*sizeof(uint8_t));
    ks->gpio_cols = (uint8_t*) malloc(col_count*sizeof(uint8_t));
    ks->keys = (uint8_t*)malloc(row_count*sizeof(uint8_t));

    uint i, gpio;

    for(i=0; i<row_count; i++) {
        gpio = gpio_rows[i];
        ks->gpio_rows[i] = gpio;
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_OUT);
    }

    for(i=0; i<col_count; i++) {
        gpio = gpio_cols[i];
        ks->gpio_cols[i] = gpio;
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_IN);
    }

    return ks;
}

void key_scan_free(key_scan_t* ks) {
    free(ks->gpio_rows);
    free(ks->gpio_cols);
    free(ks->keys);
    free(ks);
}

void key_scan_update(key_scan_t* ks) {
    memset(ks->keys, 0, ks->row_count);

    for(uint row=0; row<ks->row_count; row++) {
        uint row_gpio = ks->gpio_rows[row];
        gpio_put(row_gpio, true);

        uint8_t keys = 0;
        for(uint col=0; col<ks->col_count; col++) {
            uint col_gpio = ks->gpio_cols[col];
            keys = (keys<<1) | (gpio_get(col_gpio) ? 1 : 0);
        }

        gpio_put(row_gpio, false);

        ks->keys[row] = keys;
    }
}
