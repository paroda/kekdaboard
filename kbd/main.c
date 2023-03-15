#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "data_model.h"
#include "input_processor.h"
#include "screen_processor.h"
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
 *     - process inputs (hid_report_in key_scan, tb_motion, responses)
 *       - update usb hid_report_out
 *       - update left/right request
 *       - update system_state
 *   - role slave ->
 *     - process requests
 *       - update responses
 *   - side left ->
 *     - update LCD
 *   - side right ->
 *     - read tb_motion
 */

typedef struct {
    uint8_t gpio;
    bool on;
} kbd_led_t;

struct {
    uart_comm_t* comm;
    rtc_t* rtc;
    master_spi_t* m_spi;
    flash_t* flash;
    lcd_t* lcd;
    tb_t* tb;

    key_scan_t* ks;

    kbd_led_t led;
    kbd_led_t ledB;

    uint8_t flash_header[FLASH_PAGE_SIZE];
} kbd_hw;


uint32_t board_millis() {
    return to_ms_since_boot(get_absolute_time());
}

void do_if_elapsed(uint32_t* t_ms, uint32_t dt_ms, void* param, void(*task)(void* param)) {
    uint32_t ms = board_millis();
    if(ms > *t_ms+dt_ms) {
        task(param);
        if(dt_ms==0 || *t_ms==0) {
            *t_ms = ms;
            return;
        }
        while(*t_ms < ms) *t_ms += dt_ms;
    }
}

void toggle_led(void* param) {
    kbd_led_t* led = (kbd_led_t*) param;
    led->on = !led->on;
    gpio_put(led->gpio, led->on);
}

int32_t led_millis_to_toggle(kbd_led_t* led, kbd_led_state_t state) {
    // return milliseconds after which to toggle the led
    // return -1 to indicate skip toggle
    switch(state) {
    case kbd_led_state_OFF:
        return led->on ? 0 : -1;
    case kbd_led_state_ON:
        return led->on ? -1 : 0;
    case kbd_led_state_BLINK_LOW:    // 100(on), 900(off)
        return led->on ? 100 : 900;
    case kbd_led_state_BLINK_HIGH:   // 900, 100
        return led->on ? 900 : 100;
    case kbd_led_state_BLINK_SLOW:   // 1000, 1000
        return led->on ? 2000 : 2000;
    case kbd_led_state_BLINK_NORMAL: // 500, 500
        return led->on ? 500 : 500;
    case kbd_led_state_BLINK_FAST:   // 100, 100
        return led->on ? 50 : 50;
    }
    return -1;
}

void led_task() {
    static uint32_t led_last_ms = 0;
    int32_t led_ms = led_millis_to_toggle(&kbd_hw.led, kbd_system.led);
    if(led_ms>=0) do_if_elapsed(&led_last_ms, led_ms, &kbd_hw.led, toggle_led);

    static uint32_t ledB_last_ms = 0;
    int32_t ledB_ms = led_millis_to_toggle(&kbd_hw.ledB, kbd_system.ledB);
    if(ledB_ms>=0) do_if_elapsed(&ledB_last_ms, ledB_ms, &kbd_hw.ledB, toggle_led);
}

void key_scan_task(void* param) {
    (void)param;
    // scan key presses with kbd_hw.ks and save to sb_(left|right)_key_press
    shared_buffer_t* sb = (kbd_system.side == kbd_side_LEFT) ?
        kbd_system.sb_left_key_press : kbd_system.sb_right_key_press;
    key_scan_update(kbd_hw.ks);
    write_shared_buffer(sb, time_us_64(), kbd_hw.ks->keys);
}

void comm_task(void* param) {
    (void)param;
    peer_comm_init_cycle(kbd_system.comm);
}

void core1_main(void) {
    uart_inst_t* uart = hw_inst_UART == 0 ? uart0 : uart1;
    kbd_hw.comm = uart_comm_create(uart, hw_gpio_TX, hw_gpio_RX, kbd_system.comm);

    kbd_system.ledB = kbd_led_state_BLINK_HIGH;

    // connect to peer
    while(!kbd_system.comm->peer_ready) {
        led_task();
        peer_comm_try_peer(kbd_system.comm);
        tight_loop_contents();
    }

    kbd_system.ledB = kbd_led_state_BLINK_FAST;

    // wait for core0 to set role
    while(kbd_system.role==kbd_role_NONE) {
        led_task();
        tight_loop_contents();
    }

    while(!kbd_system.comm->peer_version) {
        led_task();
        if(kbd_system.role==kbd_role_MASTER) peer_comm_try_version(kbd_system.comm);
        tight_loop_contents();
    }

    // note the combined version
    kbd_system.version = kbd_system.side == kbd_side_LEFT ?
        (kbd_system.comm->version << 4) | kbd_system.comm->peer_version :
        (kbd_system.comm->peer_version << 4) | kbd_system.comm->version;

    kbd_system.ready = true;

    kbd_system.ledB = kbd_led_state_BLINK_NORMAL;

    while(true) {
        led_task();

        // scan key presses
        static uint32_t ks_last_ms = 0;
        do_if_elapsed(&ks_last_ms, 2, NULL, key_scan_task);

        // transfer data to/from peer
        static uint32_t comm_last_ms = 0;
        do_if_elapsed(&comm_last_ms, 2, NULL, comm_task);

        tight_loop_contents();
    }
}

void init_core1() { multicore_launch_core1(core1_main); }

void init_hw_common() {
    i2c_inst_t* i2c = hw_inst_I2C == 0 ? i2c0 : i2c1;
    kbd_hw.rtc = rtc_create(i2c, hw_gpio_SCL, hw_gpio_SDA);
    rtc_set_default_instance(kbd_hw.rtc);

    spi_inst_t* spi = hw_inst_SPI == 0 ? spi0 : spi1;
    kbd_hw.m_spi = master_spi_create(spi, 3, hw_gpio_MOSI, hw_gpio_MISO, hw_gpio_CLK);

    kbd_hw.flash = flash_create(kbd_hw.m_spi, hw_gpio_CS_flash);

    // init LEDs
    kbd_hw.led.gpio = hw_gpio_LED;
    kbd_hw.led.on = false;
    kbd_hw.ledB.gpio = 25;
    kbd_hw.ledB.on = false;

    uint8_t gpio_leds[2] = { kbd_hw.led.gpio, kbd_hw.ledB.gpio };
    for(uint i=0; i<2; i++) {
        uint8_t gpio = gpio_leds[i];
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_OUT);
        gpio_put(gpio, true);
    }

    // init key scanner
    uint8_t gpio_rows[KEY_ROW_COUNT] = hw_gpio_rows;
    uint8_t gpio_cols[KEY_COL_COUNT] = hw_gpio_cols;
    kbd_hw.ks = key_scan_create(KEY_ROW_COUNT, KEY_COL_COUNT, gpio_rows, gpio_cols);
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
    lcd_canvas_t* cv = lcd_new_canvas(240, 20, BROWN);
    // w:18x11=198 h:16, x:21-219
    lcd_canvas_text(cv, 21, 2, "Welcome Pradyumna!", &lcd_font16, WHITE, BROWN);
    lcd_display_canvas(kbd_hw.lcd, 0, 110, cv);
    lcd_free_canvas(cv);
}

void init_hw_right() {
    // setup track ball
    kbd_hw.tb = tb_create(kbd_hw.m_spi,
                          hw_gpio_CS_tb, hw_gpio_tb_MT, hw_gpio_tb_RST,
                          0, true, false, true);
}

void rtc_task(void* param) {
    (void)param;

    rtc_read_time(kbd_hw.rtc, &kbd_system.date);
    kbd_system.temperature = rtc_get_temperature(kbd_hw.rtc);
}

void lcd_display_head_task(void* param) {
    (void)param;

    char txt[8];
    uint16_t bg = BLACK;
    uint16_t fg = WHITE;
    uint16_t fg1 = YELLOW;
    uint16_t bg2 = GRAY;
    uint16_t fgcl = RED;
    uint16_t fgnl = GREEN;

    // 2 rows 240x(20+20)
    static lcd_canvas_t* cv = NULL;
    if(!cv) cv = lcd_new_canvas(240, 40, bg);

    // row1: version, time, weekday
    // version : w:11x2=22 h:16, x:10-32
    sprintf(txt, "%02x", kbd_system.version);
    lcd_canvas_text(cv, 10, 3, txt, &lcd_font16, fg, bg);
    // time (2 rows) : w:17x5=85 h:24, x:77-162, y:8
    sprintf(txt, "%02d:%02d", kbd_system.date.hour, kbd_system.date.minute);
    lcd_canvas_text(cv, 52, 3, txt, &lcd_font24, fg1, bg);
    // weekday: w:11x3=33 h:16, x:197-230
    char* wd = rtc_week[kbd_system.date.weekday%8];
    lcd_canvas_text(cv, 197, 3, wd, &lcd_font16, fg, bg);

    // row2: locksx2, temperature, MM/dd
    // locksx3: w:20x2=40 h:16, x:5-45
    lcd_canvas_rect(cv, 5, 22, 20, 16, bg2, 1, true);
    if(kbd_system.state.caps_lock)
        lcd_canvas_circle(cv, 15, 30, 5, fgcl, 1, true);
    lcd_canvas_rect(cv, 25, 22, 20, 16, bg2, 1, true);
    if(kbd_system.state.num_lock)
        lcd_canvas_circle(cv, 35, 30, 5, fgnl, 1, true);
    // temperature: w:11x2=22 h:16, x:50-72
    sprintf(txt, "%02d", kbd_system.temperature);
    lcd_canvas_text(cv, 50, 22, txt, &lcd_font16, fg, bg);
    // MM/dd: w:11x5=55 h:16, x:180-235
    sprintf(txt, "%02d/%02d", kbd_system.date.month, kbd_system.date.date);
    lcd_canvas_text(cv, 180, 22, txt, &lcd_font16, fg, bg);

    lcd_display_canvas(kbd_hw.lcd, 0, 0, cv);
}

void tb_scan_task(void* param) {
    (void)param;

    // scan tb scroll and write to sb_right_tb_motion
    kbd_tb_motion_t d = {
        .has_motion = false,
        .on_surface = false,
        .dx = 0,
        .dy = 0
    };
    d.has_motion = tb_check_motion(kbd_hw.tb, &d.on_surface, &d.dx, &d.dy);
    write_shared_buffer(kbd_system.sb_right_tb_motion, time_us_64(), &d);
}

/*
 * Processing:
 * sb_left_key_press      -->  sb_state
 * sb_right_key_press          sb_left_task_request
 * sb_right_tb_motion          sb_right_task_request
 * sb_left_task_response       hid_report_out
 * sb_right_task_response
 * hid_report_in
 */

void process_inputs(void* param) {
    (void)param;

    kbd_state_t old_state = kbd_system.state;

    // use the hid_report_in
    kbd_system.state.caps_lock = kbd_system.hid_report_in.keyboard.CapsLock;
    kbd_system.state.num_lock = kbd_system.hid_report_in.keyboard.NumLock;
    kbd_system.state.scroll_lock = kbd_system.hid_report_in.keyboard.ScrollLock;

    // read the task responses
    if(kbd_system.left_task_response_ts != kbd_system.sb_left_task_response->ts_start)
        read_shared_buffer(kbd_system.sb_left_task_response,
                           &kbd_system.left_task_response_ts, kbd_system.left_task_response);
    if(kbd_system.right_task_response_ts != kbd_system.sb_right_task_response->ts_start)
        read_shared_buffer(kbd_system.sb_right_task_response,
                           &kbd_system.right_task_response_ts, kbd_system.right_task_response);

    // read the inputs
    uint64_t ts; // unused, we read the input always
    read_shared_buffer(kbd_system.sb_left_key_press, &ts, kbd_system.left_key_press);
    read_shared_buffer(kbd_system.sb_right_key_press, &ts, kbd_system.right_key_press);
    read_shared_buffer(kbd_system.sb_right_tb_motion, &ts, &kbd_system.right_tb_motion);

    // process inputs to update the hid_report_out and generate event
    kbd_screen_event_t event = execute_input_processor();

    // send events to screens processor to process the responses and raise requests if any
    execute_screen_processor(event);

    // set the task requests
    if(kbd_system.left_task_request_ts != kbd_system.sb_left_task_request->ts_start)
        write_shared_buffer(kbd_system.sb_left_task_request,
                            kbd_system.left_task_request_ts, kbd_system.left_task_request);
    if(kbd_system.right_task_request_ts != kbd_system.sb_right_task_request->ts_start)
        write_shared_buffer(kbd_system.sb_right_task_request,
                            kbd_system.right_task_request_ts, kbd_system.right_task_request);

    // update the state if changed
    if(memcmp(&old_state, &kbd_system.state, sizeof(kbd_state_t)) != 0)
        write_shared_buffer(kbd_system.sb_state, time_us_64(), &kbd_system.state);

    usb_hid_task();
}

void process_requests() {
    // read the task request
    if(kbd_system.side == kbd_side_LEFT) {
        if(kbd_system.left_task_request_ts != kbd_system.sb_left_task_request->ts_start)
            read_shared_buffer(kbd_system.sb_left_task_request,
                               &kbd_system.left_task_request_ts, kbd_system.left_task_request);
    } else { // RIGHT
        if(kbd_system.right_task_request_ts != kbd_system.sb_right_task_request->ts_start)
            read_shared_buffer(kbd_system.sb_right_task_request,
                               &kbd_system.right_task_request_ts, kbd_system.right_task_request);
    }

    // TODO

    // set the task responses
    if(kbd_system.side == kbd_side_LEFT) {
        if(kbd_system.left_task_response_ts != kbd_system.sb_left_task_response->ts_start)
            write_shared_buffer(kbd_system.sb_left_task_response,
                                kbd_system.left_task_response_ts, kbd_system.left_task_response);
    } else { // RIGHT
        if(kbd_system.right_task_response_ts != kbd_system.sb_right_task_response->ts_start)
            write_shared_buffer(kbd_system.sb_right_task_response,
                                kbd_system.right_task_response_ts, kbd_system.right_task_response);
    }
}

int main(void) {
    // stdio_init_all();

    init_data_model();

    init_hw_common();

    usb_hid_init();

    kbd_system.led = kbd_led_state_BLINK_LOW;
    kbd_system.ledB = kbd_led_state_BLINK_LOW;

    init_core1();

    kbd_side_t side = detect_kbd_side();
    set_kbd_side(side);

    kbd_system.led = kbd_led_state_BLINK_HIGH;

    if(kbd_system.side == kbd_side_LEFT) init_hw_left();
    else init_hw_right();

    kbd_system.led = kbd_led_state_BLINK_FAST;

    // negotiate role with peer
    while((kbd_system.comm->role & 0xF0) != 0xF0) {
        usb_hid_task();

        if(kbd_system.usb_hid_state == kbd_usb_hid_state_MOUNTED && kbd_system.comm->peer_ready)
            peer_comm_try_master(kbd_system.comm, kbd_system.side==kbd_side_LEFT);
        tight_loop_contents();
    }

    kbd_system.led = kbd_led_state_BLINK_SLOW;

    // set role
    kbd_role_t role = kbd_system.comm->role == peer_comm_role_MASTER ? kbd_role_MASTER : kbd_role_SLAVE;
    set_kbd_role(role);

    // wait for core1
    while(!kbd_system.ready) {
        tight_loop_contents();
    }

    kbd_system.led = kbd_led_state_BLINK_NORMAL;

    while(true) {
        // get state
        if(kbd_system.state_ts != kbd_system.sb_state->ts_start)
            read_shared_buffer(kbd_system.sb_state, &kbd_system.state_ts, &kbd_system.state);

        // get date & temperature, once a second
        static uint32_t rtc_last_ms = 0;
        do_if_elapsed(&rtc_last_ms, 1000, NULL, rtc_task);

        if(kbd_system.side == kbd_side_LEFT) {
            // update lcd display head, @ 500 ms
            static uint32_t lcd_last_ms = 0;
            do_if_elapsed(&lcd_last_ms, 500, NULL, lcd_display_head_task);

            // set system led
            switch(kbd_system.usb_hid_state) {
            case kbd_usb_hid_state_UNMOUNTED:
                kbd_system.led = kbd_led_state_BLINK_FAST;
                break;
            case kbd_usb_hid_state_MOUNTED:
                kbd_system.led = kbd_led_state_BLINK_NORMAL;
                break;
            case kbd_usb_hid_state_SUSPENDED:
                kbd_system.led = kbd_led_state_BLINK_SLOW;
            }
        }
        else { // RIGHT
            // scan track ball scroll, @ 10 ms
            static uint32_t tb_last_ms = 0;
            do_if_elapsed(&tb_last_ms, 10, NULL, tb_scan_task);

            // set caps lock led
            kbd_system.led = kbd_system.state.caps_lock ? kbd_led_state_ON : kbd_led_state_OFF;
        }

        // process inputs(key+tb+hid_in) -> outputs(state+requests+hid_out), @ 10 ms
        if(kbd_system.role == kbd_role_MASTER) {
            static uint32_t proc_last_ms = 0;
            do_if_elapsed(&proc_last_ms, 10, NULL, process_inputs);
        }

        // execute on-demand tasks
        process_requests();

        tight_loop_contents();
    }
}
