#ifndef _KEY_SCAN_H
#define _KEY_SCAN_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t row_count;
    uint8_t col_count; // MAX 8
    uint8_t* gpio_rows;
    uint8_t* gpio_cols;

    uint8_t* keys;
} key_scan_t;

key_scan_t* key_scan_create(uint8_t row_count, uint8_t col_count,
                            uint8_t* gpio_rows, uint8_t* gpio_cols);

void key_scan_free(key_scan_t* ks);

void key_scan_update(key_scan_t* ks);

#endif
