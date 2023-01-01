#include <stdio.h>

#include "pico/stdlib.h"

#include "blink_led.h"
#include "tb_pmw3389.h"

int main(void) {
    stdio_init_all();

    sleep_ms(5000);
    printf("\nHello from PICO. Booting up..");

    // map to track-ball's pins
    uint8_t gpio_CLK = 2; // GP2 SPI0 SCK
    uint8_t gpio_MOSI = 3; // GP3 SPI0 TX
    uint8_t gpio_MISO = 4; // GP4 SPI0 RX
    uint8_t gpio_CS  = 6; // GP6
    uint8_t gpio_MT  = 0xFF; // not using, only polling is implemented
    uint8_t gpio_RST = 0xFF; // not using, just set the pin always high with 3.3V

    master_spi_t* m_spi = master_spi_create(spi0, 1, gpio_MOSI, gpio_MISO, gpio_CLK);

    tb_t* tb = tb_create(m_spi, gpio_CS, gpio_MT, gpio_RST, 0, true, false, true);

    while(true) {
        sleep_ms(1);
        led_blinking_task();

        static uint32_t start_ms = 0;
        if(start_ms == 0) start_ms = board_millis();

        static bool has_Motion=false, on_Surface=false;
        static int dX=0, dY=0;

        if(start_ms%2 == 0) {
            bool has_motion=false, on_surface=false;
            int16_t dx=0, dy=0;
            has_motion = tb_check_motion(tb, &on_surface, &dx, &dy);
            dX+=dx;
            dY+=dy;
            has_Motion = has_Motion || has_motion;
            on_Surface = on_Surface || on_surface;
        }

        if(board_millis() - start_ms > 1000) {
            spi_set_baudrate(m_spi->spi, 125*1000*1000);
            start_ms += 1000;

            printf("\nHello again from PICO. still beating..");
            print_master_spi(m_spi);
            print_tb(tb);

            printf("\nCheckMotion: @%d: motion: %d, surface: %d, dx: %d, dy: %d",
                   start_ms, has_Motion, on_Surface, dX, dY);

            has_Motion=false;
            on_Surface=false;
            dX=0;
            dY=0;
        }

        tight_loop_contents();
    }
}
