#ifndef __TB_PMW3389_H
#define __TB_PMW3389_H

#include <stdint.h>
#include <stdbool.h>

#include "master_spi.h"

extern uint8_t tb_cpi_options_count;
extern uint16_t tb_cpi_options[];

typedef struct {
    master_spi_t* m_spi;

    uint8_t spi_slave_id;

    uint8_t gpio_MT;
    uint8_t gpio_RST;

    uint8_t cpi_index;

    bool swap_XY;
    bool invert_X;
    bool invert_Y;
} tb_t;

tb_t* tb_create(master_spi_t* m_spi,
                uint8_t gpio_CS, uint8_t gpio_MT, uint8_t gpio_RST,
                uint8_t cpi_index,
                bool swap_XY, bool invert_X, bool invert_Y);

void tb_set_cpi(tb_t* tb, uint8_t cpi_index);

void tb_device_signature(tb_t* tb,
                         uint8_t* product_id, uint8_t* inverse_product_id,
                         uint8_t* srom_version, uint8_t* motion);

bool tb_check_motion(tb_t* tb, bool* on_surface, int16_t* dx, int16_t* dy);

void tb_free(tb_t* tb);

void print_tb(tb_t* tb);

#endif
