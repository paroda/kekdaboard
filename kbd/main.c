#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "data_model.h"
#include "hw_config.h"
#include "uart_comm.h"
#include "rtc_ds3231.h"
#include "master_spi.h"
#include "flash_w25qxx.h"
#include "lcd_st7789.h"
#include "tb_pmw3389.h"

/*
 * Process Overview
 *
 * - init data_model
 * - init hw (except left(lcd), right(tb) and core1(uart))
 * - detect side
 * - init hw left(lcd) or right(tb)
 * - init core1
 *   - init hw uart
 *   - init comm
 *   - loop
 *     - scan keys 6x7
 *     - init transfer cycle
 * - init usb
 *   - wait for usb or comm to be ready
 *     - setup role (master or slave)
 * - loop tasks
 *   - role master ->
 *     - process inputs (key_scan, ball_scroll, responses)
 *       - send usb hid_report
 *       - update system_state
 *       - set request
 *   - side left ->
 *     - write LCD
 *   - side right ->
 *     - read ball_scroll
 */

struct {
    uart_comm_t* comm;
    rtc_t* rtc;
    master_spi_t* m_spi;
    flash_t* flash;
    lcd_t* lcd;
    tb_t* tb;

    uint8_t flash_header[FLASH_PAGE_SIZE];
} kbd_hw;

static void core1_entry(void) {
    kbd_hw.comm = uart_comm_create(hw_inst_UART, hw_gpio_TX, hw_gpio_RX, kbd_system.comm);

    // connect to peer
    while(true) {

    }

    while(true) {

        tight_loop_contents();
    }
}

static void init_core1() {
    multicore_launch_core1(core1_entry);
}

static void init_hw_common() {
    kbd_hw.rtc = rtc_create(hw_inst_I2C, hw_gpio_SCL, hw_gpio_SDA);

    kbd_hw.m_spi = master_spi_create(hw_inst_SPI, 3, hw_gpio_MOSI, hw_gpio_MISO, hw_gpio_CLK);

    kbd_hw.flash = flash_create(kbd_hw.m_spi, hw_gpio_CS_flash);
}

static void detect_kbd_side() {
    memset(kbd_hw.flash_header, 0, FLASH_PAGE_SIZE);
    flash_read(kbd_hw.flash, 0, kbd_hw.flash_header, FLASH_PAGE_SIZE);
    kbd_system.side = (kbd_hw.flash_header[hw_flash_addr_side] == hw_flash_side_left) ?
        kbd_side_LEFT : kbd_side_RIGHT;
}

static void init_hw_left() {
    // setup lcd
    kbd_hw.lcd = lcd_create(kbd_hw.m_spi,
                            hw_gpio_CS_lcd, hw_gpio_lcd_DC, hw_gpio_lcd_RST, hw_gpio_lcd_BL,
                            240, 240,
                            lcd_orient_Normal);

    lcd_clear(kbd_hw.lcd, BROWN);
    lcd_canvas_t* cv = lcd_new_canvas(180, 30, BROWN);
    lcd_canvas_text(cv, 30, 105, "Welcome Pradyumna!", &lcd_font16, WHITE, BROWN);
    lcd_display_canvas(kbd_hw.lcd, 10, 7, cv);
    lcd_free_canvas(cv);
}

static void init_hw_right() {
    // setup track ball
    kbd_hw.tb = tb_create(kbd_hw.m_spi,
                          hw_gpio_CS_tb, hw_gpio_tb_MT, hw_gpio_tb_RST,
                          0, true, false, true);
}

static void init_usb() {

}

int main(void) {
    stdio_init_all();

    init_data_model();

    init_hw_common();

    detect_kbd_side();

    if(kbd_system.side == kbd_side_LEFT) init_hw_left();
    else init_hw_right();

    init_core1();

    init_usb();

    while(true) {

        tight_loop_contents();
    }
}
