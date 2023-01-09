#ifndef __MASTER_SPI_H
#define __MASTER_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "hardware/spi.h"
    typedef struct {
        spi_inst_t* spi;
        uint8_t slave_count;
        uint8_t slave_count_max;

        uint8_t active_slave_id; // 0xFF is reserved to indicate none

        uint8_t gpio_MOSI;
        uint8_t gpio_MISO;
        uint8_t gpio_CLK;

        uint8_t* gpio_CSn;
        uint8_t* MODEn;
        uint32_t* BAUDn;
    } master_spi_t;

    master_spi_t* master_spi_create(spi_inst_t* spi, uint8_t slave_count_max,
                                    uint8_t gpio_MOSI, uint8_t MISO, uint8_t gpio_CLK);

    void master_spi_free(master_spi_t* m_spi);

    // set mode=0, baud=0 for defaults
    uint8_t master_spi_add_slave(master_spi_t* m_spi, uint8_t gpio_CS, uint8_t mode, uint32_t baud);

    void master_spi_set_baud(master_spi_t* m_spi, uint8_t slave_id);

    void master_spi_select_slave(master_spi_t* m_spi, uint8_t slave_id);

    void master_spi_release_slave(master_spi_t* m_spi, uint8_t slave_id);

    /*
     * NOTE: You must select and release the slave before and after calling below operations
     */

    void master_spi_write8(master_spi_t* m_spi, const uint8_t* src, size_t len);

    void master_spi_write16(master_spi_t* m_spi, const uint16_t* src, size_t len);

    void master_spi_write8_read8(master_spi_t* m_spi, const uint8_t* src, uint8_t* dst, size_t len);

    void master_spi_write16_read16(master_spi_t* m_spi, const uint16_t* src, uint16_t* dst, size_t len);

    void master_spi_read8(master_spi_t* m_spi, uint8_t* dst, size_t len);

    void master_spi_read16(master_spi_t* m_spi, uint16_t* dst, size_t len);

    // simple register read/write methods with devices, which use the MSB bit to indicate read/write
    void master_spi_read_register(master_spi_t* m_spi, uint8_t reg, uint8_t* dst, uint16_t len);

    void master_spi_write_register(master_spi_t* m_spi, uint8_t reg, uint8_t data);

    // for debugging
    void print_master_spi(master_spi_t* m_spi);

#ifdef __cplusplus
}
#endif

#endif
