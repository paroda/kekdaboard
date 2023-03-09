#ifndef _HW_CONFIG_H
#define _HW_CONFIG_H

#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

#define hw_inst_UART uart0
#define hw_gpio_TX 0
#define hw_gpio_RX 1

#define hw_inst_I2C i2c1
#define hw_gpio_SCL 27
#define hw_gpio_SDA 26

#define hw_inst_SPI spi0
#define hw_gpio_CLK 2
#define hw_gpio_MOSI 3
#define hw_gpio_MISO 4

#define hw_gpio_CS_lcd 6
#define hw_gpio_lcd_BL 8
#define hw_gpio_lcd_DC 9
#define hw_gpio_lcd_RST 0xFF // use software RESET

#define hw_gpio_CS_tb 6
#define hw_gpio_tb_MT 0xFF  // not used
#define hw_gpio_tb_RST 0xFF // not used

#define hw_gpio_CS_flash 5
#define hw_gpio_CS_sd_card 7

#define hw_gpio_LED 28

#define hw_gpio_row1 15
#define hw_gpio_row2 14
#define hw_gpio_row3 13
#define hw_gpio_row4 12
#define hw_gpio_row5 11
#define hw_gpio_row6 10

#define hw_gpio_col1 22
#define hw_gpio_col2 21
#define hw_gpio_col3 20
#define hw_gpio_col4 19
#define hw_gpio_col5 18
#define hw_gpio_col6 17
#define hw_gpio_col7 16

// FLASH 1st page (256 bytes) (permanent)
// byes 0x00-0x0F 16bytes for my name
#define hw_flash_name "Pradyumna"
// designate side (left: 0xF0, right:0xF1)
#define hw_flash_addr_side 0x10
#define hw_flash_side_left 0xF0
#define hw_flash_side_right 0xF1

#endif
