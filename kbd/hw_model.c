#include "hw_config.h"
#include "hw_model.h"

kbd_hw_t kbd_hw;

static void flash_store_read(uint32_t addr, uint8_t* buf, size_t len) {
    flash_read(kbd_hw.flash, addr, buf, len);
}

static void flash_store_page_program(uint32_t addr, const uint8_t* buf, size_t len) {
    flash_page_program(kbd_hw.flash, addr, buf, len);
}

static void flash_store_sector_erase(uint32_t addr) {
    flash_sector_erase(kbd_hw.flash, addr);
}

uint32_t board_millis() {
    return us_to_ms(time_us_64());
}

void do_if_elapsed(uint32_t* t_ms, uint32_t dt_ms, void* param, void(*task)(void* param)) {
    uint32_t ms = board_millis();
    if(ms >= *t_ms+dt_ms) {
        task(param);
        if(dt_ms==0 || *t_ms==0) {
            *t_ms = ms;
            return;
        }
        while(*t_ms < ms) *t_ms += dt_ms;
    }
}

void init_hw_core1(peer_comm_config_t* comm) {
    uart_inst_t* uart = hw_inst_UART == 0 ? uart0 : uart1;
    kbd_hw.comm = uart_comm_create(uart, hw_gpio_TX, hw_gpio_RX, comm);
}

void init_hw_common() {
    uint i;

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
    for(i=0; i<2; i++) {
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

void lcd_update_backlight(uint8_t level) {
    static uint8_t old = 0;
    if(old!=level) {
        lcd_set_backlight_level(kbd_hw.lcd, level); // 30%
        old = level;
    }
}

void init_hw_left() {
    // setup lcd
    kbd_hw.lcd = lcd_create(kbd_hw.m_spi,
                            hw_gpio_CS_lcd, hw_gpio_lcd_DC, hw_gpio_lcd_RST, hw_gpio_lcd_BL,
                            240, 240,
                            lcd_orient_Normal);
    lcd_update_backlight(30);
    lcd_clear(kbd_hw.lcd, LCD_BODY_BG);
    kbd_hw.lcd_body = lcd_new_canvas(240, 200, LCD_BODY_BG);
    lcd_show_welcome();
}

void init_hw_right() {

    // tb_t* tb_create(master_spi_t* m_spi,
    //                 uint8_t gpio_CS, uint8_t gpio_MT, uint8_t gpio_RST,
    //                 uint16_t cpi,
    //                 bool swap_XY, bool invert_X, bool invert_Y);

    // setup track ball
    kbd_hw.tb = tb_create(kbd_hw.m_spi,
                          hw_gpio_CS_tb, hw_gpio_tb_MT, hw_gpio_tb_RST,
                          KBD_TB_CPI_DEFAULT,
                          true, false, false);

    // setup key switch leds
    pio_hw_t* pio = hw_inst_PIO == 0 ? pio0 : pio1;
    kbd_hw.led_pixel = led_pixel_create(pio, hw_inst_PIO_SM, hw_gpio_led_left_DI, hw_led_pixel_count); // right_DI is next consecutive GPIO
}

void init_flash_datasets(flash_dataset_t** flash_datasets) {
    uint8_t i, ids[KBD_CONFIG_SCREEN_COUNT];
    for(i=0; i<KBD_CONFIG_SCREEN_COUNT; i++)
        ids[i] = kbd_config_screens[i];
    flash_create_store(KBD_CONFIG_SCREEN_COUNT, ids, flash_datasets,
                       flash_store_read, flash_store_page_program, flash_store_sector_erase);
}

void load_flash_datasets(flash_dataset_t** flash_datasets) {
    uint8_t i;
    for(i=0; i<KBD_CONFIG_SCREEN_COUNT; i++)
        flash_store_load(flash_datasets[i]);
}

void lcd_display_body() {
    lcd_display_canvas(kbd_hw.lcd, 0, 40, kbd_hw.lcd_body);
}

void lcd_display_body_canvas(uint16_t xs, uint16_t ys, lcd_canvas_t *canvas) {
    lcd_display_canvas(kbd_hw.lcd, xs, ys+40, canvas);
}

void lcd_show_welcome() {
    lcd_canvas_clear(kbd_hw.lcd_body);
    // w:18x11=198 h:16, x:21-219
    lcd_canvas_text(kbd_hw.lcd_body, 21, 92, "Welcome Pradyumna!",
                    &lcd_font16, LCD_BODY_FG, LCD_BODY_BG);
    lcd_display_body();
}
