#include <stdlib.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "master_spi.h"

#define SLAVE_ID_NONE 255

master_spi_t* master_spi_create(spi_inst_t* spi, uint8_t slave_count_max,
                                uint8_t gpio_MOSI, uint8_t gpio_MISO, uint8_t gpio_CLK) {
    master_spi_t* m_spi = (master_spi_t*) malloc(sizeof(master_spi_t));
    m_spi->spi = spi;
    m_spi->slave_count = 0;
    m_spi->slave_count_max = slave_count_max;
    m_spi->active_slave_id = SLAVE_ID_NONE;
    m_spi->write_mode = master_spi_write_mode_none;
    m_spi->gpio_MOSI = gpio_MOSI;
    m_spi->gpio_MISO = gpio_MISO;
    m_spi->gpio_CLK = gpio_CLK;
    m_spi->gpio_CSn = (uint8_t*) malloc(slave_count_max);

    spi_init(m_spi->spi, 125 * 1000 * 1000);
    gpio_set_function(m_spi->gpio_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(m_spi->gpio_MISO, GPIO_FUNC_SPI);
    gpio_set_function(m_spi->gpio_CLK, GPIO_FUNC_SPI);

    return m_spi;
}

void master_spi_free(master_spi_t* m_spi) {
    spi_deinit(m_spi->spi);

    free(m_spi->gpio_CSn);
    free(m_spi);
}

uint8_t master_spi_add_slave(master_spi_t* m_spi, uint8_t gpio_CS) {
    uint8_t slave_id = m_spi->slave_count++;
    m_spi->gpio_CSn[slave_id] = gpio_CS;

    gpio_init(gpio_CS);
    gpio_set_dir(gpio_CS, GPIO_OUT);
    gpio_put(gpio_CS, true);

    /* printf("\nmaster_spi_add_slave, slave_id=%d, gpio_CSn[slave_id]%d", slave_id, m_spi->gpio_CSn[slave_id]); */
    return slave_id;
}

void master_spi_begin_send(master_spi_t* m_spi, uint8_t slave_id) {
    if(m_spi->active_slave_id == slave_id) return;
    gpio_put(m_spi->gpio_CSn[slave_id], false);
    m_spi->active_slave_id = slave_id;
    sleep_us(1);
    /* printf("\nmaster_spi_begin_send, active_slave_id=%d", m_spi->active_slave_id); */
}

void master_spi_end_send(master_spi_t* m_spi, uint8_t slave_id) {
    if(m_spi->active_slave_id != slave_id) return;
    gpio_put(m_spi->gpio_CSn[slave_id], true);
    m_spi->active_slave_id = SLAVE_ID_NONE;
    sleep_us(1);
    /* printf("\nmaster_spi_end_send, active_slave_id=%d", m_spi->active_slave_id); */
}

static inline void master_spi_set_write_mode(master_spi_t* m_spi, master_spi_write_mode_t write_mode) {
    if(m_spi->write_mode == write_mode) return;
    if(write_mode == master_spi_write_mode_8bit)
        spi_set_format(m_spi->spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    else
        spi_set_format(m_spi->spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    m_spi->write_mode = write_mode;
    /* printf("\nmaster_spi_set_write_mode, mode=%d", m_spi->write_mode); */
}

void master_spi_write8(master_spi_t* m_spi, const uint8_t* src, size_t len) {
    master_spi_set_write_mode(m_spi, master_spi_write_mode_8bit);
    spi_write_blocking(m_spi->spi, src, len);
    /* printf("\nmaster_spi_write8, len=%d", len); */
}

void master_spi_write16(master_spi_t* m_spi, const uint16_t* src, size_t len) {
    master_spi_set_write_mode(m_spi, master_spi_write_mode_16bit);
    spi_write16_blocking(m_spi->spi, src, len);
    /* printf("\nmaster_spi_write16, len=%d", len); */
}

void print_master_spi(master_spi_t* m_spi) {
    printf("\n\nMaster SPI:");
    printf("\nm_spi->slave_count, %d", m_spi->slave_count);
    printf("\nm_spi->slave_count_max, %d", m_spi->slave_count_max);
    printf("\nm_spi->active_slave_id, %d", m_spi->active_slave_id);
    printf("\nm_spi->write_mode, %d", m_spi->write_mode);
    printf("\nm_spi->gpio_MOSI, %d", m_spi->gpio_MOSI);
    printf("\nm_spi->gpio_MISO, %d", m_spi->gpio_MISO);
    printf("\nm_spi->gpio_CLK, %d", m_spi->gpio_CLK);
    for(int i=0; i<m_spi->slave_count; i++)
        printf("\nm_spi->gpio_CSn[%d], %d", i, m_spi->gpio_CSn[i]);
}
