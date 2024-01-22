#ifndef _HW_MODEL_H
#define _HW_MODEL_H

#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"

#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "screen_model.h"
#include "key_layout.h"
#include "flash_store.h"

#include "key_scan.h"
#include "uart_comm.h"
#include "rtc_ds3231.h"
#include "master_spi.h"
#include "flash_w25qxx.h"
#include "lcd_st7789.h"
#include "led_pixel.h"
#include "tb_pmw3389.h"

#define LCD_BODY_BG ORCHID
#define LCD_BODY_FG WHITE

typedef struct {
    uint8_t gpio;
    bool on;
} kbd_led_t;

typedef struct {
    uart_comm_t* comm;
    rtc_t* rtc;
    master_spi_t* m_spi;
    flash_t* flash;
    lcd_t* lcd;
    led_pixel_t* led_pixel;
    tb_t* tb;

    key_scan_t* ks;

    kbd_led_t led;  // status led - one on each side
    kbd_led_t ledB; // board led - one on each pico

    lcd_canvas_t* lcd_body; // 240x200 at 0,40 - 240,240
} kbd_hw_t;

extern kbd_hw_t kbd_hw;

uint32_t board_millis();

void do_if_elapsed(uint32_t* t_ms, uint32_t dt_ms, void* param, void(*task)(void* param));

void init_hw_core1(peer_comm_config_t* comm);

void init_hw_common();

void lcd_update_backlight(uint8_t level);

void init_hw_left();

void init_hw_right();

void init_flash_datasets(flash_dataset_t** flash_datasets);

void load_flash_datasets(flash_dataset_t** flash_datasets);

void lcd_display_body();

void lcd_display_body_canvas(uint16_t xs, uint16_t ys, lcd_canvas_t *canvas);

void lcd_show_welcome();

#endif
