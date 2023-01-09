#include <stdlib.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "master_spi.h"

#define SLAVE_ID_NONE 255
#define SPI_DEFAULT_BAUD (62500 * 1000) // maximum posible 62.5 MHz

#ifdef __cplusplus
extern "C" {
#endif

    master_spi_t* master_spi_create(spi_inst_t* spi, uint8_t slave_count_max,
                                    uint8_t gpio_MOSI, uint8_t gpio_MISO, uint8_t gpio_CLK) {
        master_spi_t* m_spi = (master_spi_t*) malloc(sizeof(master_spi_t));
        m_spi->spi = spi;
        m_spi->slave_count = 0;
        m_spi->slave_count_max = slave_count_max;
        m_spi->active_slave_id = SLAVE_ID_NONE;

        m_spi->gpio_MOSI = gpio_MOSI;
        m_spi->gpio_MISO = gpio_MISO;
        m_spi->gpio_CLK = gpio_CLK;

        m_spi->gpio_CSn = (uint8_t*) malloc(slave_count_max);
        m_spi->MODEn = (uint8_t*) malloc(slave_count_max);
        m_spi->BAUDn = (uint32_t*) malloc(slave_count_max * sizeof(uint32_t));

        spi_init(m_spi->spi, SPI_DEFAULT_BAUD);
        gpio_set_function(m_spi->gpio_MOSI, GPIO_FUNC_SPI);
        gpio_set_function(m_spi->gpio_MISO, GPIO_FUNC_SPI);
        gpio_set_function(m_spi->gpio_CLK, GPIO_FUNC_SPI);

        return m_spi;
    }

    void master_spi_free(master_spi_t* m_spi) {
        spi_deinit(m_spi->spi);

        free(m_spi->gpio_CSn);
        free(m_spi->MODEn);
        free(m_spi->BAUDn);
        free(m_spi);
    }

    uint8_t master_spi_add_slave(master_spi_t* m_spi, uint8_t gpio_CS, uint8_t mode, uint32_t baud) {
        uint8_t slave_id = m_spi->slave_count++;
        m_spi->gpio_CSn[slave_id] = gpio_CS;
        m_spi->MODEn[slave_id] = mode;
        m_spi->BAUDn[slave_id] = baud==0? SPI_DEFAULT_BAUD : baud;

        gpio_init(gpio_CS);
        gpio_set_dir(gpio_CS, GPIO_OUT);
        gpio_put(gpio_CS, true);

        /* printf("\nmaster_spi_add_slave, slave_id=%d, gpio_CSn[slave_id]%d", slave_id, m_spi->gpio_CSn[slave_id]); */
        return slave_id;
    }

    void master_spi_set_baud(master_spi_t* m_spi, uint8_t slave_id) {
        spi_set_baudrate(m_spi->spi, m_spi->BAUDn[slave_id]);
        gpio_put(m_spi->gpio_CSn[slave_id], false);
        sleep_us(1);
        gpio_put(m_spi->gpio_CSn[slave_id], true);
        sleep_us(1);
    }

    void master_spi_select_slave(master_spi_t* m_spi, uint8_t slave_id) {
        if(m_spi->active_slave_id == slave_id) return;
        m_spi->active_slave_id = slave_id;

        gpio_put(m_spi->gpio_CSn[slave_id], false);
        sleep_us(1);
        /* printf("\nmaster_spi_select_slave, active_slave_id=%d", m_spi->active_slave_id); */
    }

    void master_spi_release_slave(master_spi_t* m_spi, uint8_t slave_id) {
        if(m_spi->active_slave_id != slave_id) return;
        m_spi->active_slave_id = SLAVE_ID_NONE;

        gpio_put(m_spi->gpio_CSn[slave_id], true);
        sleep_us(1);
        /* printf("\nmaster_spi_release_slave, active_slave_id=%d", m_spi->active_slave_id); */
    }

    static inline void master_spi_set_format(master_spi_t* m_spi, uint8_t data_bits) {
        spi_cpol_t cpol;
        spi_cpha_t cpha;
        switch(m_spi->MODEn[m_spi->active_slave_id]) {
        case 1:
            cpol = SPI_CPOL_0;
            cpha = SPI_CPHA_1;
            break;
        case 2:
            cpol = SPI_CPOL_1;
            cpha = SPI_CPHA_0;
            break;
        case 3:
            cpol = SPI_CPOL_1;
            cpha = SPI_CPHA_1;
            break;
        case 0:
        default:
            cpol = SPI_CPOL_0;
            cpha = SPI_CPHA_0;
            break;
        }

        spi_set_format(m_spi->spi, data_bits, cpol, cpha, SPI_MSB_FIRST);
    }

    void master_spi_write8(master_spi_t* m_spi, const uint8_t* src, size_t len) {
        master_spi_set_format(m_spi, 8);
        spi_write_blocking(m_spi->spi, src, len);
        /* printf("\nmaster_spi_write8, len=%d", len); */
    }

    void master_spi_write16(master_spi_t* m_spi, const uint16_t* src, size_t len) {
        master_spi_set_format(m_spi, 16);
        spi_write16_blocking(m_spi->spi, src, len);
        /* printf("\nmaster_spi_write16, len=%d", len); */
    }

    void master_spi_write8_read8(master_spi_t* m_spi, const uint8_t* src, uint8_t* dst, size_t len) {
        master_spi_set_format(m_spi, 8);
        spi_write_read_blocking(m_spi->spi, src, dst, len);
    }

    void master_spi_write16_read16(master_spi_t* m_spi, const uint16_t* src, uint16_t* dst, size_t len) {
        master_spi_set_format(m_spi, 16);
        spi_write16_read16_blocking(m_spi->spi, src, dst, len);
    }

    void master_spi_read8(master_spi_t* m_spi, uint8_t* dst, size_t len) {
        master_spi_set_format(m_spi, 8);
        spi_read_blocking(m_spi->spi, 0, dst, len);
        /* printf("\nmaster_spi_read8, len=%d", len); */
    }

    void master_spi_read16(master_spi_t* m_spi, uint16_t* dst, size_t len) {
        master_spi_set_format(m_spi, 16);
        spi_read16_blocking(m_spi->spi, 0, dst, len);
        /* printf("\nmaster_spi_read16, len=%d", len); */
    }

    void master_spi_read_register(master_spi_t* m_spi, uint8_t reg, uint8_t* dst, uint16_t len) {
        // we send the device the register we want to read first
        // then subsequently read from the device. The register is auto incrementing
        // so we don't need to keep sending the register we want, just the first.
        reg |= 0x80; // set MSB bit 1 to indicate read operation
        master_spi_write8(m_spi, &reg, 1);
        sleep_ms(1);
        master_spi_read8(m_spi, dst, len);
        sleep_ms(1);
    }

    void master_spi_write_register(master_spi_t* m_spi, uint8_t reg, uint8_t data) {
        uint8_t buf[] = { reg & 0x7F, data }; // remove the read bit (MSB) from reg
        master_spi_write8(m_spi, buf, 2);
        sleep_ms(1);
    }

    void print_master_spi(master_spi_t* m_spi) {
        printf("\n\nMaster SPI:");
        printf("\nbuadrate: %d", spi_get_baudrate(m_spi->spi));
        printf("\nm_spi->slave_count, %d", m_spi->slave_count);
        printf("\nm_spi->slave_count_max, %d", m_spi->slave_count_max);
        printf("\nm_spi->active_slave_id, %d", m_spi->active_slave_id);
        printf("\nm_spi->gpio_MOSI, %d", m_spi->gpio_MOSI);
        printf("\nm_spi->gpio_MISO, %d", m_spi->gpio_MISO);
        printf("\nm_spi->gpio_CLK, %d", m_spi->gpio_CLK);
        for(int i=0; i<m_spi->slave_count; i++)
            printf("\nm_spi->gpio_CSn,MODEn,BAUDn[%d], %d,%d,%d", i,
                   m_spi->gpio_CSn[i], m_spi->MODEn[i], m_spi->BAUDn[i]);
    }

#ifdef __cplusplus
}
#endif
