#ifndef _KEY_SCAN_H
#define _KEY_SCAN_H

#include <stdint.h>
#include <stdbool.h>

#define KEY_ROW_COUNT 6
#define KEY_COL_COUNT 7

typedef struct {
    uint8_t gpio_rows[KEY_ROW_COUNT];
    uint8_t gpio_cols[KEY_COL_COUNT];

    uint8_t keys[KEY_ROW_COUNT];
} key_scan_t;

key_scan_t* key_scan_create(uint8_t* gpio_rows, uint8_t* gpio_cols);

void key_scan_free(key_scan_t* ks);

void key_scan_update(key_scan_t* ks);

#endif
