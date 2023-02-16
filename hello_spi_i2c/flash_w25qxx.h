#ifndef __FLASH_W25QXX_H
#define __FLASH_W25QXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "master_spi.h"

#define W25Q80_SIZE 1048576 // 8M-bit / 1M-byte
#define W25Q80_ID 0xEF4014
#define W25Q16_SIZE 2097152 // 16M-bit / 2M-byte
#define W25Q16_ID 0xEF4015
#define W25Q32_SIZE 4194304 // 32M-bit / 4M-byte / 64 x 64K blocks
#define W25Q32_ID 0xEF4016

#define FLASH_PAGE_SIZE 256
#define FLASH_SECTOR_SIZE 4096 // 4K-byte

    typedef struct {
        master_spi_t* m_spi;

        uint8_t spi_slave_id;

    } flash_t;

    flash_t* flash_create(master_spi_t* m_spi, uint8_t gpio_CS);

    void flash_free(flash_t* f);

    void flash_read(flash_t* f, uint32_t addr, uint8_t* buf, size_t len);

    void flash_page_program(flash_t* f, uint32_t addr, const uint8_t* buf, size_t len);

    void flash_sector_erase(flash_t* f, uint32_t addr);

    void flash_block_erase_32K(flash_t* f, uint32_t addr);

    void flash_block_erase_64K(flash_t* f, uint32_t addr);

    void flash_chip_erase_all(flash_t* f);

    uint32_t flash_get_id(flash_t* f);

#ifdef __cplusplus
}
#endif

#endif
