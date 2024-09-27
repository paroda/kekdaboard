#ifndef _HW_MODEL_H
#define _HW_MODEL_H

#include "hw_config.h"

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

typedef struct {
    bool wl_led; // access via the wireless chip?
    uint8_t gpio;
    bool on;
} kbd_led_t;

typedef struct {
    master_spi_t* m_spi;

#ifdef KBD_NODE_AP
    flash_t* flash;
#endif

#ifdef KBD_NODE_LEFT
    rtc_t* rtc;
    lcd_t* lcd;
    lcd_canvas_t* lcd_body; // 240x200; 0,40 - 240,240
#endif

#ifdef KBD_NODE_RIGHT
    tb_t* tb;
#endif

    kbd_led_t ledB; // board led

#ifdef KBD_NODE_AP
    kbd_led_t led_left;
    kbd_led_t led_right;
#endif

#if defined(KBD_NODE_LEFT) || defined(KBD_NODE_RIGHT)
    kbd_led_t led;  // status led

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
