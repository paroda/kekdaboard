#ifndef _HW_CONFIG_H
#define _HW_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#include "main.h"

#define hw_ap_name "kekdaboard"
#define hw_ap_password "3picow"
#define hw_ap_ip "192.168.4.1"
#define hw_left_ip "192.168.4.2"
#define hw_right_ip "192.168.4.3"
#define hw_udp_port 81
#define hw_tcp_port 82

#define hw_led_pixel_count 42 // each side

#define hw_inst_SPI 0
#define hw_gpio_CLK 2
#define hw_gpio_MOSI 3
#define hw_gpio_MISO 4

//////////////////////////////////////////////////////////// ACCESS POINT

#ifdef KBD_NODE_AP

#define hw_gpio_CS_flash 5
#define hw_gpio_LED_LEFT 10
#define hw_gpio_LED_RIGHT 21

#endif

//////////////////////////////////////////////////////////// LEFT NODE

#ifdef KBD_NODE_LEFT

#define hw_inst_I2C 1
#define hw_gpio_SCL 27
#define hw_gpio_SDA 26

#define hw_gpio_CS_sd_card 5

#define hw_gpio_LED 28

// lcd
#define hw_gpio_CS_lcd 7
#define hw_gpio_lcd_BL 8
#define hw_gpio_lcd_DC 9
#define hw_gpio_lcd_RST 0xFF // use software RESET

// key scan layout
#define hw_gpio_rows  {10, 11, 12, 13, 14, 15} // 6 rows
#define hw_gpio_cols {16, 17, 18, 19, 20, 21, 22} // 7 cols

// key pixels
#define hw_inst_PIO 0
#define hw_inst_PIO_SM 0
#define hw_gpio_led_DI 6
#define hw_key_led_mapping {                    \
        { 0,  1,  2,  3,  4,  5,  6},           \
        {13, 12, 11, 10,  9,  8,  7}            \
        {14, 15, 16, 17, 18, 19, 20}            \
        {27, 26, 25, 24, 23, 22, 21}            \
        {28, 29, 30, 31, 32, 33, 34}            \
        {41, 40, 39, 38, 37, 36, 35}            \
    }

#endif

//////////////////////////////////////////////////////////// RIGHT NODE

#ifdef KBD_NODE_RIGHT

#define hw_gpio_LED 28

// trackball
#define hw_gpio_CS_tb 7
#define hw_gpio_tb_MT 0xFF  // not used
#define hw_gpio_tb_RST 0xFF // not used

// key scan layout
#define hw_gpio_rows  {21, 20, 19, 18, 17, 16} // 6 rows
#define hw_gpio_cols {9, 10, 11, 12, 13, 14, 15} // 7 cols

// key pixels
#define hw_inst_PIO 0
#define hw_inst_PIO_SM 0
#define hw_gpio_led_DI 6
#define hw_key_led_mapping {                    \
        { 6,  5,  4,  3,  2,  1,  0},           \
        { 7,  8,  9, 10, 11, 12, 13}            \
        {20, 19, 18, 17, 16, 15, 14}            \
        {21, 22, 23, 24, 25, 26, 27}            \
        {34, 33, 32, 31, 30, 29, 28}            \
        {35, 36, 37, 38, 39, 40, 41}            \
    }

#endif


#endif
