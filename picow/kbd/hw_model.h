#ifndef _HW_MODEL_H
#define _HW_MODEL_H

#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"

#include "main.h"

#include "hardware/spi.h"
#include "hardware/gpio.h"

#ifdef KBD_NODE_LEFT
#include "hardware/i2c.h"
#endif

#include "screen_model.h"
#include "util/master_spi.h"

#ifdef KBD_NODE_AP
#include "key_layout.h"
#include "util/flash_store.h"
#include "util/flash_w25qxx.h"
#endif

#ifdef KBD_NODE_LEFT
#include "util/key_scan.h"
#include "util/led_pixel.h"
#include "util/rtc_ds3231.h"
#include "util/lcd_st7789.h"
#endif

#ifdef KBD_NODE_RIGHT
#include "util/key_scan.h"
#include "util/led_pixel.h"
#include "util/tb_pmw3389.h"
#endif

#ifdef KBD_NODE_LEFT
#define LCD_BODY_BG ORCHID
#define LCD_BODY_FG WHITE
#endif

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
typedef struct {
    uint8_t gpio;
    bool on;
} kbd_led_t;
#endif

typedef struct {
    master_spi_t* m_spi;

#ifdef KBD_NODE_AP
    flash_t* flash;
    bool ledB_on; // board led
#endif

#ifdef KBD_NODE_LEFT
    rtc_t* rtc;
    lcd_t* lcd;
    lcd_canvas_t* lcd_body; // 240x200; 0,40 - 240,240
#endif

#ifdef KBD_NODE_RIGHT
    tb_t* tb;
#endif

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
    kbd_led_t led;  // status led
    kbd_led_t ledB; // board led

    led_pixel_t* led_pixel;
    key_scan_t* ks;
#endif
} kbd_hw_t;

extern kbd_hw_t kbd_hw;

uint32_t board_millis();

void do_if_elapsed(uint32_t* t_ms, uint32_t dt_ms, void* param, void(*task)(void* param));

#ifdef KBD_NODE_AP

void init_flash_datasets(flash_dataset_t** flash_datasets);

void load_flash_datasets(flash_dataset_t** flash_datasets);

#endif

#ifdef KBD_NODE_LEFT

void lcd_update_backlight(uint8_t level);

void lcd_display_body();

void lcd_display_body_canvas(uint16_t xs, uint16_t ys, lcd_canvas_t *canvas);

void lcd_show_welcome();

#endif

void init_hw_core0();

void init_hw_core1();

#endif
