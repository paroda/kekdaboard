#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "usb_descriptors.h"

#include "data_model.h"
#include "hw_config.h"
#include "key_scan.h"
#include "uart_comm.h"
#include "rtc_ds3231.h"
#include "master_spi.h"
#include "flash_w25qxx.h"
#include "lcd_st7789.h"
#include "tb_pmw3389.h"
#include "usb_hid.h"

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
 *       - update system_state
 *       - send usb hid_report
 *       - set request
 *   - role slave ->
 *     - process requests
 *       - set responses
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

    key_scan_t* ks;

    uint8_t gpio_LED;
    uint8_t gpio_LEDB;

    uint8_t flash_header[FLASH_PAGE_SIZE];
} kbd_hw;


uint32_t board_millis() {
    return to_ms_since_boot(get_absolute_time());
}

void update_led(uint8_t gpio, kbd_led_state_t state, uint64_t* ms) {
    if(board_millis() > next_ms) next_ms += 1;
    else return;

    // TODO
}

void led_task() {
    static uint32_t next_ms = 0;
    if(next_ms==0) next_ms = board_millis();

    update_led(kbd_hw.gpio_LED, kbd_system.led, &next_ms);
}


void ledB_task() {
    static uint32_t next_ms = 0;
    if(next_ms==0) next_ms = board_millis();

    update_led(kbd_hw.gpio_LEDB, kbd_system.ledB, &next_ms);
}

void key_scan_task() {
    static uint32_t next_ms = 0;
    if(next_ms==0) next_ms = board_millis();

    if(board_millis() > next_ms) next_ms += 2;
    else return;

    // TODO
}

void comm_task() {
    static uint32_t next_ms = 0;
    if(next_ms==0) next_ms = board_millis();

    if(board_millis() > next_ms) next_ms += 2;
    else return;

    peer_comm_init_cycle(kbd_system.comm);
}

void core1_main(void) {
    kbd_hw.comm = uart_comm_create(hw_inst_UART, hw_gpio_TX, hw_gpio_RX, kbd_system.comm);

    // connect to peer
    while(!kbd_system.comm->peer_ready) {
        sleep_ms(10);
        peer_comm_try_peer(kbd_system.comm);
    }

    while(kbd_system.role==kbd_role_NONE) sleep_ms(10); // wait for core0 to set role

    kbd_system.ready = true;

    while(true) {
        led_task();
        ledB_task();
        key_scan_task();
        comm_task();
    }
}

void init_core1() { multicore_launch_core1(core1_main); }

void init_hw_common() {
    kbd_hw.rtc = rtc_create(hw_inst_I2C, hw_gpio_SCL, hw_gpio_SDA);

    kbd_hw.m_spi = master_spi_create(hw_inst_SPI, 3, hw_gpio_MOSI, hw_gpio_MISO, hw_gpio_CLK);

    kbd_hw.flash = flash_create(kbd_hw.m_spi, hw_gpio_CS_flash);

    // init LEDs
    kbd_hw.gpio_LED = hw_gpio_LED;
    kbd_hw.gpio_LEDB = 25;

    uint8_t gpio_leds[2] = { kbd_hw.gpio_LED, kbd_hw.gpio_LEDB };
    for(uint i=0; i<2; i++) {
        uint8_t gpio = gpio_leds[i];
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_OUT);
        gpio_put(gpio, true);
    }

    // init key scanner
    uint8_t gpio_rows[KEY_ROW_COUNT] = hw_gpio_rows;
    uint8_t gpio_cols[KEY_COL_COUNT] = hw_gpio_cols;
    kbd_hw.ks = key_scan_create(gpio_rows, gpio_cols);
}

kbd_side_t detect_kbd_side() {
    memset(kbd_hw.flash_header, 0, FLASH_PAGE_SIZE);
    flash_read(kbd_hw.flash, 0, kbd_hw.flash_header, FLASH_PAGE_SIZE);
    return (kbd_hw.flash_header[hw_flash_addr_side] == hw_flash_side_left) ?
        kbd_side_LEFT : kbd_side_RIGHT;
}

void init_hw_left() {
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

void init_hw_right() {
    // setup track ball
    kbd_hw.tb = tb_create(kbd_hw.m_spi,
                          hw_gpio_CS_tb, hw_gpio_tb_MT, hw_gpio_tb_RST,
                          0, true, false, true);
}

void lcd_task() {
    static uint32_t next_ms = 0;
    if(next_ms==0) next_ms = board_millis();

    if(board_millis() > next_ms) next_ms += 10;
    else return;

    // TODO
}

void tb_task() {
    static uint32_t next_ms = 0;
    if(next_ms==0) next_ms = board_millis();

    if(board_millis() > next_ms) next_ms += 10;
    else return;

    // TODO
}

void process_inputs() {
    static uint32_t next_ms = 0;
    if(next_ms==0) next_ms = board_millis();

    if(board_millis() > next_ms) next_ms += 10;
    else return;

    // TODO key_press + ball_scroll + response -> system_state + request

    usb_hid_task();
}

void process_requests() {
    // TODO
}

void update_system() {
    // TODO
}


int main(void) {
    stdio_init_all();

    init_data_model();

    init_hw_common();

    usb_hid_init();

    init_core1();

    kbd_side_t side = detect_kbd_side();
    set_kbd_side(side);

    if(kbd_system.side == kbd_side_LEFT) init_hw_left();
    else init_hw_right();

    // negotiate role with peer
    while(kbd_system.comm->role & 0xF0 != 0xF0) {
        sleep_ms(100);
        if(kbd_system.usb_hid_state == kbd_usb_hid_state_MOUNTED && kbd_system.comm->peer_ready)
            peer_comm_try_master(kbd_system.comm, kbd_system.side==kbd_side_LEFT);
    }

    // set role
    kbd_role_t role = kbd_system.comm->role == peer_comm_role_MASTER ? kbd_role_MASTER : kbd_role_SLAVE;
    set_kbd_role(role);

    while(!kbd_system.ready) sleep_ms(10); // wait for core1

    while(true) {
        if(kbd_system.side == kbd_side_LEFT) lcd_task();
        else tb_task();

        if(kbd_system.role == kbd_role_MASTER) process_inputs();

        update_system();

        process_requests();
    }
}
