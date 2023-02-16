#include <stdlib.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include "lcd_st7789.h"

/*
 * Commands list for ST7789 driver (only a small subset)
 */

#define LCD_CMD_NOP       0x00
#define LCD_CMD_SWRESET   0x01
#define LCD_CMD_SLPIN     0x10
#define LCD_CMD_SLPOUT    0x11
#define LCD_CMD_PTLON     0x12
#define LCD_CMD_NORON     0x13
#define LCD_CMD_INVOFF    0x20
#define LCD_CMD_INVON     0x21
#define LCD_CMD_DISPOFF   0x28
#define LCD_CMD_DISPON    0x29
#define LCD_CMD_CASET     0x2A
#define LCD_CMD_RASET     0x2B
#define LCD_CMD_RAMWR     0x2C
#define LCD_CMD_RAMRD     0x2E
#define LCD_CMD_PTLAR     0x30
#define LCD_CMD_VSCRDEF   0x33
#define LCD_CMD_MADCTL    0x36
#define LCD_CMD_VSCRSADD  0x37
#define LCD_CMD_COLMOD    0x3A
#define LCD_CMD_RAMWRC    0x3C
#define LCD_CMD_RAMRDC    0x3E

#define GPIO_NONE 0xFF

/*
 * PWM wrap value of 65500 with 125Mz clock result in 1.9 kHz frequency.
 * With further clock divider 19 1/16 it would be close to 100Hz.
 * This would emit a pulse at every 10ms.
 * The width of the pulse would be defined as a ratio of level to wrap.
 * So the duty cycle with a level of 32750 (= 65500/2) comes out as 50%.
 * And the pulse would be 5ms wide.
 */
#define LCD_BL_LEVEL_MAX 65500u // wrap at 65500 (max 100 in 16 bits)

#ifdef __cplusplus
extern "C" {
#endif

    static void lcd_init_backlight(lcd_t* lcd, uint8_t gpio_BL) {
        // initialize PWM for backlight
        lcd->gpio_BL = gpio_BL;
        lcd->pwm_slice_num_BL = pwm_gpio_to_slice_num(gpio_BL);
        lcd->pwm_chan_BL = pwm_gpio_to_channel(gpio_BL);
        lcd->backlight_level = 100u;

        gpio_set_function(gpio_BL, GPIO_FUNC_PWM);

        pwm_set_clkdiv_int_frac(lcd->pwm_slice_num_BL, 19, 1); // 50Hz:38,2, 100Hz:19,1
        pwm_set_wrap(lcd->pwm_slice_num_BL, LCD_BL_LEVEL_MAX);
        pwm_set_chan_level(lcd->pwm_slice_num_BL, lcd->pwm_chan_BL, LCD_BL_LEVEL_MAX);
        pwm_set_enabled(lcd->pwm_slice_num_BL, true);
        /* printf("\nlcd_init_backlight, gpio_BL=%d", lcd->gpio_BL); */
    }

    static inline void lcd_set_cmd_mode(lcd_t* lcd) {
        if(lcd->on_DC == false) return;
        gpio_put(lcd->gpio_DC, false);
        lcd->on_DC = false;
        sleep_us(1);
        /* printf("\nlcd_set_cmd_mode, on_DC=%d", lcd->on_DC); */
    }

    static inline void lcd_set_data_mode(lcd_t* lcd) {
        if(lcd->on_DC == true) return;
        gpio_put(lcd->gpio_DC, true);
        lcd->on_DC = true;
        sleep_us(1);
        /* printf("\nlcd_set_data_mode, on_DC=%d", lcd->on_DC); */
    }

    // assumes master_spi slave already selected
    static void lcd_send_cmd(lcd_t* lcd, uint8_t cmd) {
        lcd_set_cmd_mode(lcd);
        master_spi_write8(lcd->m_spi, &cmd, 1);
        sleep_us(1);
        /* printf("\nlcd_send_cmd, cmd=%d", cmd); */
    }

    // assumes master_spi slave already selected
    static void lcd_send_data_bytes(lcd_t* lcd, const uint8_t* data, size_t len) {
        lcd_set_data_mode(lcd);
        master_spi_write8(lcd->m_spi, data, len);
        sleep_us(1);
        /* printf("\nlcd_send_data_bytes, len=%d", len); */
    }

    // assumes master_spi slave already selected
    static void lcd_send_data_words(lcd_t* lcd, const uint16_t* data, size_t len) {
        lcd_set_data_mode(lcd);
        master_spi_write16(lcd->m_spi, data, len);
        sleep_us(1);
        /* printf("\nlcd_send_data_words, len=%d", len); */
    }

    // assumes master_spi slave already selected
    static void lcd_set_window(lcd_t* lcd,
                               uint16_t xs, uint16_t ys, uint16_t width, uint16_t height) {
        xs += lcd->origin_x;
        ys += lcd->origin_y;

        uint16_t xe = xs + width - 1;
        uint8_t x_data[] = {
            xs >> 8, xs & 0xff,
            xe >> 8, xe & 0xff
        };
        lcd_send_cmd(lcd, LCD_CMD_CASET);
        lcd_send_data_bytes(lcd, x_data, 4);

        uint16_t ye = ys + height - 1;
        uint8_t y_data[] = {
            ys >> 8, ys & 0xff,
            ye >> 8, ye & 0xff
        };
        lcd_send_cmd(lcd, LCD_CMD_RASET);
        lcd_send_data_bytes(lcd, y_data, 4);
        /* printf("\nlcd_set_window, xs=%d, ys=%d, width=%d, height=%d", xs, ys, width, height); */
    }

    static void lcd_command(lcd_t* lcd, uint8_t cmd, const uint8_t* data, size_t len) {
        master_spi_select_slave(lcd->m_spi, lcd->spi_slave_id);
        lcd_send_cmd(lcd, cmd);
        lcd_send_data_bytes(lcd, data, len);
        master_spi_release_slave(lcd->m_spi, lcd->spi_slave_id);
        /* printf("\nlcd_command, cmd=%d, data len=%d", cmd, len); */
    }

    static void lcd_reset(lcd_t* lcd) {
        if(lcd->gpio_RST==GPIO_NONE) {
            // Software reset
            lcd_command(lcd, LCD_CMD_SWRESET, NULL, 0);
            sleep_ms(150);
            // Sleep out
            lcd_command(lcd, LCD_CMD_SLPOUT, NULL, 0);
            sleep_ms(50);
            /* printf("\nlcd_reset, software reset"); */
        } else {
            // Hardware reset
            gpio_put(lcd->gpio_RST, true);
            sleep_ms(100);
            gpio_put(lcd->gpio_RST, false);
            sleep_ms(100);
            gpio_put(lcd->gpio_RST, true);
            sleep_ms(100);
            /* printf("\nlcd_reset, hardware reset, %d", lcd->gpio_RST); */
        }
    }

    static void lcd_init_device(lcd_t* lcd, uint8_t gpio_CS,
                                uint8_t gpio_DC, uint8_t gpio_RST, uint8_t gpio_BL) {
        // register the lcd as a slave with master spi
        lcd->spi_slave_id = master_spi_add_slave(lcd->m_spi, gpio_CS, 0, 0);
        master_spi_set_baud(lcd->m_spi, lcd->spi_slave_id);

        // Initialize DC pin
        lcd->gpio_DC = gpio_DC;
        gpio_init(lcd->gpio_DC);
        gpio_set_dir(lcd->gpio_DC, GPIO_OUT);
        gpio_put(lcd->gpio_DC, true);
        lcd->on_DC = true;

        // Initialize RST pin when not GPIO_NONE (use software reset)
        lcd->gpio_RST = gpio_RST;
        if(lcd->gpio_RST != GPIO_NONE) {
            gpio_init(lcd->gpio_RST);
            gpio_set_dir(lcd->gpio_RST, GPIO_OUT);
            gpio_put(lcd->gpio_RST, true);
        }

        // Initialize backlight PWM
        lcd_init_backlight(lcd, gpio_BL);

        sleep_ms(100);
        /* printf("\nlcd_init_device, gpio_DC=%d, on_DC=%d, gpio_RST=%d", lcd->gpio_DC, lcd->on_DC, lcd->gpio_RST); */
    }

    static void lcd_resolution(lcd_t* lcd, uint16_t width, uint16_t height, lcd_orient_t orient) {
        uint8_t memory_access;
        // D7 MY, D6 MX, D5 MV, D4 ML, D3 RGB, D2 MH, D1 -, D0 -
        // normal      : 0 0 0 0 0000 = 0x00
        // rotate left : 1 0 1 0 0000 = 0xA0
        // rotate right: 0 1 1 0 0000 = 0x60

        lcd->orient = orient;
        if(orient == lcd_orient_Normal) {
            lcd->width = width;
            lcd->height = height;
            lcd->origin_x = 0;
            lcd->origin_y = 0;
            memory_access = 0x00;
        } else {
            lcd->width = height;
            lcd->height = width;
            lcd->origin_x = (orient == lcd_orient_Left) ? LCD_RES_HEIGHT - height : 0;
            lcd->origin_y = (orient == lcd_orient_Right) ? LCD_RES_WIDTH - width : 0;
            memory_access = (orient == lcd_orient_Left)? 0xA0 : 0x60;
        }

        // set the read/write scan direction and RGB format
        lcd_command(lcd, LCD_CMD_MADCTL, &memory_access, 1);
        /* printf("\nlcd_resolution, width=%d, height=%d, orient=%d", lcd->width, lcd->height, lcd->orient); */
    }

    lcd_t* lcd_create(master_spi_t* m_spi,
                      uint8_t gpio_CS, uint8_t gpio_DC, uint8_t gpio_RST, uint8_t gpio_BL,
                      uint16_t width, uint16_t height, lcd_orient_t orient) {
        lcd_t* lcd = (lcd_t*) malloc(sizeof(lcd_t));
        lcd->m_spi = m_spi;

        // Prepare the SPI port
        lcd_init_device(lcd, gpio_CS, gpio_DC, gpio_RST, gpio_BL);

        // Reset
        lcd_reset(lcd);

        // Set the resolution and scanning method of the screen
        lcd_resolution(lcd, width, height, orient);

        // Interface Pixel Format
        // - RGB interface color format = 65K of RGB interface
        // - Control interface color format = 16bit/pixel
        lcd_command(lcd, LCD_CMD_COLMOD, (uint8_t[]){ 0x55 }, 1);
        sleep_ms(10);

        // Display Inversion On
        lcd_command(lcd, LCD_CMD_INVON, NULL, 0);
        sleep_ms(10);

        // Sleep out
        lcd_command(lcd, LCD_CMD_SLPOUT, NULL, 0);
        sleep_ms(50);

        // Normal Display Mode On
        lcd_command(lcd, LCD_CMD_NORON, NULL, 0);
        sleep_ms(10);

        // Display On
        lcd_command(lcd, LCD_CMD_DISPON, NULL, 0);
        sleep_ms(10);

        return lcd;
    }

    void lcd_free(lcd_t* lcd) {
        free(lcd);
    }

    void lcd_orient(lcd_t* lcd, lcd_orient_t orient) {
        master_spi_set_baud(lcd->m_spi, lcd->spi_slave_id);
        lcd->orient = orient;
        if(lcd->orient == lcd_orient_Normal)
            lcd_resolution(lcd, lcd->width, lcd->height, orient);
        else
            lcd_resolution(lcd, lcd->height, lcd->width, orient);
        /* printf("\nlcd_orient, %d", lcd->orient); */
    }

    void lcd_clear(lcd_t* lcd, uint16_t color) {
        master_spi_set_baud(lcd->m_spi, lcd->spi_slave_id);
        uint16_t row[lcd->width];
        for(uint i=0; i < lcd->width; i++)
            row[i] = color;

        master_spi_select_slave(lcd->m_spi, lcd->spi_slave_id);
        lcd_set_window(lcd, 0, 0, lcd->width, lcd->height);
        lcd_send_cmd(lcd, LCD_CMD_RAMWR);
        for(uint i=0; i < lcd->height; i++)
            lcd_send_data_words(lcd, row, lcd->width);
        master_spi_release_slave(lcd->m_spi, lcd->spi_slave_id);
        /* printf("\nlcd_clear"); */
    }

    void lcd_display_canvas(lcd_t* lcd, uint16_t xs, uint16_t ys, lcd_canvas_t* canvas) {
        master_spi_set_baud(lcd->m_spi, lcd->spi_slave_id);
        master_spi_select_slave(lcd->m_spi, lcd->spi_slave_id);
        lcd_set_window(lcd, xs, ys, canvas->width, canvas->height);
        lcd_send_cmd(lcd, LCD_CMD_RAMWR);
        lcd_send_data_words(lcd, canvas->buff, canvas->width * canvas->height);
        master_spi_release_slave(lcd->m_spi, lcd->spi_slave_id);
    }

    void lcd_set_backlight_level(lcd_t* lcd, uint8_t level) {
        if(level > 100) level=100; // max 100%
        lcd->backlight_level = level;
        pwm_set_chan_level(lcd->pwm_slice_num_BL, lcd->pwm_chan_BL, (LCD_BL_LEVEL_MAX/100u)*level);
        /* printf("\nlcd_set_backlight_level, %d", lcd->backlight_level); */
    }

    void print_lcd(lcd_t* lcd) {
        printf("\n\nLCD:");
        printf("\nlcd->spi_slave_id = %d", lcd->spi_slave_id);
        printf("\nlcd->gpio_DC = %d", lcd->gpio_DC);
        printf("\nlcd->gpio_RST = %d", lcd->gpio_RST);
        printf("\nlcd->gpio_BL = %d", lcd->gpio_BL);
        printf("\nlcd->backlight_level = %d", lcd->backlight_level);
        printf("\nlcd->on_DC = %d", lcd->on_DC);
        printf("\nlcd->pwm_slice_num_BL = %d", lcd->pwm_slice_num_BL);
        printf("\nlcd->pwm_chan_BL = %d", lcd->pwm_chan_BL);
        printf("\nlcd->width = %d", lcd->width);
        printf("\nlcd->height = %d", lcd->height);
        printf("\nlcd->orient = %d", lcd->orient);
        printf("\nlcd->origin_x = %d", lcd->origin_x);
        printf("\nlcd->origin_y = %d", lcd->origin_y);
    }

#ifdef __cplusplus
}
#endif
