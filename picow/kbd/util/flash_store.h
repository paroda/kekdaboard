#ifndef _FLASH_STORE_H
#define _FLASH_STORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define FLASH_DATASET_SIZE 32

typedef struct {
    uint8_t data[FLASH_DATASET_SIZE];
    uint8_t id;    // 1-255

    bool need_flash_init; // not present in flash
    uint16_t pos; // current position in allocated block
    uint32_t addr; // address of allocated block in flash
} flash_dataset_t;

void flash_create_store(uint8_t count, uint8_t* ids, flash_dataset_t** fds,
                        void (*read)(uint32_t addr, uint8_t* buf, size_t len),
                        void (*page_program)(uint32_t addr, const uint8_t* buf, size_t len),
                        void (*sector_erase)(uint32_t addr));

void flash_free_dataset(flash_dataset_t* fd);

void flash_store_load(flash_dataset_t* fd);

void flash_store_save(flash_dataset_t* fd);

#endif
