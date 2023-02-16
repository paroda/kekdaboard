#include <stdlib.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "flash_w25qxx.h"

/*
 * Note: The command Program Page only applies the 0 bits.
 *       Hence, the flash should be erased beforehand to set the target sector to all 1-bits.
 *       Otherwise, programmed bytes would be corrupted by previous 0 bits.
 */

#define FLASH_CMD_PAGE_PROGRAM    0x02
#define FLASH_CMD_READ            0x03
#define FLASH_CMD_STATUS          0x05
#define FLASH_CMD_WRITE_EN        0x06
#define FLASH_CMD_SECTOR_ERASE    0x20
#define FLASH_CMD_BLOCK_ERASE_32K 0x52
#define FLASH_CMD_BLOCK_ERASE_64K 0xD8
#define FLASH_CMD_CHIP_ERASE      0xC7 // set all the bytes of flash to 0xFF
#define FLASH_CMD_GET_ID          0x9F

#define FLASH_STATUS_BUSY_MASK 0x01

#define FLASH_MODE_W25QXX 0 // Mode 0 or 3 supported by W25QXX

/*
 * @ 31.25 MHz approximate operation duration
 * :  one page read - 200 us
 * :  4 bytes read - 90 us
 * :  one page write - 300 - 600 us, depending on count of 0-bits
 * :  4 bytes write - 250 us (varies with count of 0-bits)
 * :  one sector (4K) erase - 90 ms
 * :  one 32K block erase - 130 ms
 * :  one 64K block erase - 160 ms
 * :  whole chip erase - 6-7 seconds
 *
 * At the max baud rate 62.5 MHz data gets corrupted.
 * Using next lower value 31.25 MHz.
 */
#define FLASH_BAUD_W25QXX (32 * 1000 * 1000) // SPI will choose closest possible 31.25 MHz

static void __not_in_flash_func(flash_write_enable)(flash_t* f) {
    master_spi_select_slave(f->m_spi, f->spi_slave_id);

    uint8_t cmd = FLASH_CMD_WRITE_EN;
    master_spi_write8(f->m_spi, &cmd, 1);

    master_spi_release_slave(f->m_spi, f->spi_slave_id);
}

static void __not_in_flash_func(flash_wait_done)(flash_t* f) {
    uint8_t status;
    do {
        master_spi_select_slave(f->m_spi, f->spi_slave_id);
        uint8_t buf[2] = {FLASH_CMD_STATUS, 0};
        master_spi_write8_read8(f->m_spi, buf, buf, 2);
        master_spi_release_slave(f->m_spi, f->spi_slave_id);
        status = buf[1];
        if(status & FLASH_STATUS_BUSY_MASK) sleep_us(100); else return;
    } while(true);
}

void __not_in_flash_func(flash_read)(flash_t* f, uint32_t addr, uint8_t* buf, size_t len) {
    master_spi_set_baud(f->m_spi, f->spi_slave_id);
    master_spi_select_slave(f->m_spi, f->spi_slave_id);
    uint8_t cmd[4] = {
        FLASH_CMD_READ,
        addr >> 16,
        addr >> 8,
        addr
    };
    master_spi_write8(f->m_spi, cmd, 4);
    master_spi_read8(f->m_spi, buf, len);
    master_spi_release_slave(f->m_spi, f->spi_slave_id);
}

void __not_in_flash_func(flash_page_program)(flash_t* f, uint32_t addr, const uint8_t* buf, size_t len) {
    master_spi_set_baud(f->m_spi, f->spi_slave_id);
    uint8_t cmd[4] = {
        FLASH_CMD_PAGE_PROGRAM,
        addr >> 16,
        addr >> 8,
        addr
    };
    flash_write_enable(f);
    master_spi_select_slave(f->m_spi, f->spi_slave_id);
    master_spi_write8(f->m_spi, cmd, 4);
    master_spi_write8(f->m_spi, buf, len);
    master_spi_release_slave(f->m_spi, f->spi_slave_id);
    flash_wait_done(f);
}

void __not_in_flash_func(flash_sector_erase)(flash_t* f, uint32_t addr) {
    master_spi_set_baud(f->m_spi, f->spi_slave_id);
    uint8_t cmd[4] = {
        FLASH_CMD_SECTOR_ERASE,
        addr >> 16,
        addr >> 8,
        addr
    };
    flash_write_enable(f);
    master_spi_select_slave(f->m_spi, f->spi_slave_id);
    master_spi_write8(f->m_spi, cmd, 4);
    master_spi_release_slave(f->m_spi, f->spi_slave_id);
    flash_wait_done(f);
}

void __not_in_flash_func(flash_block_erase_32K)(flash_t* f, uint32_t addr) {
    master_spi_set_baud(f->m_spi, f->spi_slave_id);
    uint8_t cmd[4] = {
        FLASH_CMD_BLOCK_ERASE_32K,
        addr >> 16,
        addr >> 8,
        addr
    };
    flash_write_enable(f);
    master_spi_select_slave(f->m_spi, f->spi_slave_id);
    master_spi_write8(f->m_spi, cmd, 4);
    master_spi_release_slave(f->m_spi, f->spi_slave_id);
    flash_wait_done(f);
}

void __not_in_flash_func(flash_block_erase_64K)(flash_t* f, uint32_t addr) {
    master_spi_set_baud(f->m_spi, f->spi_slave_id);
    uint8_t cmd[4] = {
        FLASH_CMD_BLOCK_ERASE_64K,
        addr >> 16,
        addr >> 8,
        addr
    };
    flash_write_enable(f);
    master_spi_select_slave(f->m_spi, f->spi_slave_id);
    master_spi_write8(f->m_spi, cmd, 4);
    master_spi_release_slave(f->m_spi, f->spi_slave_id);
    flash_wait_done(f);
}

void __not_in_flash_func(flash_chip_erase_all)(flash_t* f) {
    master_spi_set_baud(f->m_spi, f->spi_slave_id);
    uint8_t cmd[1] = {
        FLASH_CMD_CHIP_ERASE
    };
    flash_write_enable(f);
    master_spi_select_slave(f->m_spi, f->spi_slave_id);
    master_spi_write8(f->m_spi, cmd, 1);
    master_spi_release_slave(f->m_spi, f->spi_slave_id);
    flash_wait_done(f);
}

uint32_t flash_get_id(flash_t* f) {
    master_spi_set_baud(f->m_spi, f->spi_slave_id);
    uint8_t id[3] = {FLASH_CMD_GET_ID, 0, 0};

    master_spi_select_slave(f->m_spi, f->spi_slave_id);
    master_spi_write8(f->m_spi, id, 1);
    master_spi_read8(f->m_spi, id, 3);
    master_spi_release_slave(f->m_spi, f->spi_slave_id);

    // W25QXX should report EF 40 14(1MB)/15(2MB)/16(4MB)

    uint32_t n = id[0];
    n = n<<8 | id[1];
    n = n<<8 | id[2];

    return n;
}

flash_t* flash_create(master_spi_t* m_spi, uint8_t gpio_CS) {
    flash_t* f = (flash_t*) malloc(sizeof(flash_t));
    f->m_spi = m_spi;

    // register the flash as a slave with master spi, MODE 0, 62.5 MHz
    f->spi_slave_id = master_spi_add_slave(f->m_spi, gpio_CS, FLASH_MODE_W25QXX, FLASH_BAUD_W25QXX);
    master_spi_set_baud(f->m_spi, f->spi_slave_id);

    return f;
}

void flash_free(flash_t* f) {
    free(f);
}
