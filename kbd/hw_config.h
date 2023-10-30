#ifndef _HW_CONFIG_H
#define _HW_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define hw_inst_UART 0
#define hw_gpio_TX 0
#define hw_gpio_RX 1

#define hw_inst_I2C 1
#define hw_gpio_SCL 27
#define hw_gpio_SDA 26

#define hw_inst_SPI 0
#define hw_gpio_CLK 2
#define hw_gpio_MOSI 3
#define hw_gpio_MISO 4

#define hw_gpio_CS_lcd 6
#define hw_gpio_lcd_BL 8
#define hw_gpio_lcd_DC 9
#define hw_gpio_lcd_RST 0xFF // use software RESET

#define hw_inst_PIO 0
#define hw_inst_PIO_SM 0
#define hw_gpio_led_left_DI 7
#define hw_gpio_led_right_DI 8
#define hw_led_pixel_count 42

#define hw_gpio_CS_tb 6
#define hw_gpio_tb_MT 0xFF  // not used
#define hw_gpio_tb_RST 0xFF // not used

#define hw_gpio_CS_flash 5
#define hw_gpio_CS_sd_card 7

#define hw_gpio_LED 28

#define hw_gpio_rows  {15, 14, 13, 12, 11, 10} // 6 rows
#define hw_gpio_cols {22, 21, 20, 19, 18, 17, 16} // 7 cols

#define hw_key_leds_left { \
    { 0,  1,  2,  3,  4,  5,  6}, \
    {13, 12, 11, 10,  9,  8,  7} \
    {14, 15, 16, 17, 18, 19, 20} \
    {27, 26, 25, 24, 23, 22, 21} \
    {28, 29, 30, 31, 32, 33, 34} \
    {35, 36, 37, 38, 39, 40, 41} \
} // FIX last row

#define hw_key_leds_right { \
    { 6,  5,  4,  3,  2,  1,  0}, \
    { 7,  8,  9, 10, 11, 12, 13} \
    {20, 19, 18, 17, 16, 15, 14} \
    {21, 22, 23, 24, 25, 26, 27} \
    {34, 33, 32, 31, 30, 29, 28} \
    {35, 36, 37, 38, 39, 40, 41} \
} // FIX last row

#endif
