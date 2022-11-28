#ifndef __MASTER_SPI_H
#define __MASTER_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "hardware/spi.h"

    typedef enum {
        master_spi_write_mode_8bit,
        master_spi_write_mode_16bit
    } master_spi_write_mode_t;

    typedef struct {
        spi_inst_t* spi;
        uint8_t slave_count;
        uint8_t slave_count_max;

        uint8_t active_slave_id; // 255 is reserved to indicate none

        master_spi_write_mode_t write_mode;

        uint8_t gpio_DIN;
        uint8_t gpio_CLK;
        uint8_t* gpio_CSn;
    } master_spi_t;

    master_spi_t* master_spi_create(spi_inst_t* spi, uint8_t slave_count_max,
                                    uint8_t gpio_DIN, uint8_t gpio_CLK);

    void master_spi_free(master_spi_t* m_spi);

    uint8_t master_spi_add_slave(master_spi_t* m_spi, uint8_t gpio_CS);

    void master_spi_begin_send(master_spi_t* m_spi, uint8_t slave_id);

    void master_spi_end_send(master_spi_t* m_spi, uint8_t slave_id);

    void master_spi_write8(master_spi_t* m_spi, const uint8_t* src, size_t len);

    void master_spi_write16(master_spi_t* m_spi, const uint16_t* src, size_t len);

#ifdef __cplusplus
}
#endif

#endif
