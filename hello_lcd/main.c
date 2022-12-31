#include <stdio.h>

#include "pico/stdlib.h"

#include "blink_led.h"
#include "lcd_st7789.h"

int main(void) {
    stdio_init_all();

    sleep_ms(6000);
    printf("\nHello from PICO. Booting up..");

    // map to lcd's pins
    uint8_t gpio_CLK = 2; // GP2 SPI0 SCK
    uint8_t gpio_MOSI = 3; // GP3 SPI0 TX
    uint8_t gpio_MISO = 4; // GP4 SPI0 RX
    uint8_t gpio_DC  = 6; // GP6
    uint8_t gpio_CS  = 7; // GP7
    uint8_t gpio_RST = 0xFF; // using software reset instead of hardware reset
    uint8_t gpio_BL = 28; // GP28

    /* gpio_init(27); */
    /* gpio_set_dir(27, GPIO_OUT); */
    /* gpio_put(27, true); */

    master_spi_t* m_spi = master_spi_create(spi0, 1, gpio_MOSI, gpio_MISO, gpio_CLK);

    // do not swap the width and height for rotated view
    // it already takes care of that
    lcd_t* lcd = lcd_create(m_spi,
                            gpio_CS, gpio_DC, gpio_RST, gpio_BL,
                            /* 240, 320, */
                            /* 200, 200, */
                            240, 240,
                            lcd_orient_Normal);

    lcd_clear(lcd, WHITE);

    lcd_canvas_t* cv = lcd_new_canvas(50, 50, YELLOW);
    lcd_canvas_line(cv, 0, 0, 49, 49, GREEN, 3, false);
    lcd_canvas_line(cv, 0, 49, 49, 0, MAGENTA, 3, true);
    lcd_canvas_line(cv, 0, 25, 49, 25, BROWN, 1, true);
    lcd_display_canvas(lcd, 100, 100, cv);

    lcd_canvas_t* cv2 = lcd_new_canvas(180, 30, GRAY);
    lcd_canvas_text(cv2, 3, 3, "Hello World!", &lcd_font16 , RED, WHITE);
    lcd_display_canvas(lcd, 10, 50, cv2);

    while(true) {
        sleep_ms(100);
        led_blinking_task();

        static uint32_t start_ms = 0;
        if(start_ms == 0) start_ms = board_millis();

        static uint8_t bl_level = 10;
        bl_level = (bl_level + 5) % 100;
        lcd_set_backlight_level(lcd, bl_level);

        if(board_millis() - start_ms > 3000) {
            start_ms += 3000;
            lcd_orient(lcd, (lcd->orient+1)%3);
            lcd_clear(lcd, WHITE);
            lcd_display_canvas(lcd, 100, 100, cv);
            lcd_display_canvas(lcd, 10, 50, cv2);

            printf("\nHello again from PICO. still beating..");
            print_master_spi(m_spi);
            print_lcd(lcd);
        }

        tight_loop_contents();
    }
}
