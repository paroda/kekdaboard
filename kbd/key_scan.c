#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"

#include "key_scan.h"

key_scan_t* key_scan_create(uint8_t* gpio_rows, uint8_t* gpio_cols) {
    key_scan_t* ks = (key_scan_t*) malloc(sizeof(key_scan_t));

    uint i, gpio;

    for(i=0; i<KEY_ROW_COUNT; i++) {
        gpio = gpio_rows[i];
        ks->gpio_rows[i] = gpio;
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_OUT);
    }

    for(i=0; i<KEY_COL_COUNT; i++) {
        gpio = gpio_cols[i];
        ks->gpio_cols[i] = gpio;
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_IN);
    }

    return ks;
}

void key_scan_free(key_scan_t* ks) {
    free(ks);
}

void key_scan_update(key_scan_t* ks) {
    memset(ks->keys, 0, KEY_ROW_COUNT);

    for(uint row=0; row<KEY_ROW_COUNT; row++) {
        uint row_gpio = ks->gpio_rows[row];
        gpio_put(row_gpio, true);

        uint8_t keys = 0;
        for(uint col=0; col<KEY_COL_COUNT; col++) {
            uint col_gpio = ks->gpio_cols[col];
            keys<<=1;
            if(gpio_get(col_gpio)) keys |= 1;
        }
        ks->keys[row] = keys;
    }
}
