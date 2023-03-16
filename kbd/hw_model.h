#ifndef _HW_MODEL_H
#define _HW_MODEL_H

#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "key_scan.h"
#include "uart_comm.h"
#include "rtc_ds3231.h"
#include "master_spi.h"
#include "flash_w25qxx.h"
#include "lcd_st7789.h"
#include "tb_pmw3389.h"

#define LCD_BODY_BG BROWN
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
    tb_t* tb;

    key_scan_t* ks;

    kbd_led_t led;
    kbd_led_t ledB;

    lcd_canvas_t* lcd_body; // 240x200 at 0,40 - 240,240

    uint8_t flash_header[FLASH_PAGE_SIZE];
} kbd_hw_t;

extern kbd_hw_t kbd_hw;

void lcd_display_body();

void lcd_show_welcome();

#endif
