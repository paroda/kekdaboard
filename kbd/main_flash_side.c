#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

#include "hw_config.h"
#include "master_spi.h"
#include "flash_w25qxx.h"

void printbuf(uint8_t* buf, size_t len) {
    for(int i=0; i<len; i++) {
        if(i%16 == 0)
            printf("\n%02x", buf[i]);
        else if(i%4 == 0)
            printf("   %02x", buf[i]);
        else
            printf(".%02x", buf[i]);
    }
}

bool check_name(uint8_t* buf) {
    char* flash_name = hw_flash_name;
    for(int i=0; i<hw_flash_addr_side; i++) {
        if(flash_name[i]==0) return true;
        if(buf[i]!=flash_name[i]) return false;
    }
    return true;
}

void set_side(flash_t* f, uint8_t side) {
    printf("\n\nSetting flash side: %d\n", side);

    flash_sector_erase(f, 0);

    uint8_t buf[FLASH_PAGE_SIZE];
    memset(buf, 0, FLASH_PAGE_SIZE);

    int i=0;
    char* flash_name = hw_flash_name;
    while(flash_name[i]!=0) {
        buf[i] = flash_name[i];
        i++;
    }

    buf[hw_flash_addr_side] = side;

    flash_page_program(f, 0, buf, FLASH_PAGE_SIZE);
}

uint8_t get_target_flash_side();

int main(void) {
    stdio_init_all();

    sleep_ms(5000);
    printf("\nHello from PICO. Booting up..");

    spi_inst_t* spi = hw_inst_SPI == 0 ? spi0 : spi1;
    master_spi_t* m_spi = master_spi_create(spi, 1, hw_gpio_MOSI, hw_gpio_MISO, hw_gpio_CLK);

    flash_t* f = flash_create(m_spi, hw_gpio_CS_flash);

    while(true) {
        sleep_ms(1000);

        uint8_t buf[FLASH_PAGE_SIZE];
        memset(buf, 0, FLASH_PAGE_SIZE);

        uint32_t id = flash_get_id(f);
        printf("\n\nFlash device ID: %#010x", id);

        flash_read(f, 0, buf, FLASH_PAGE_SIZE);
        printf("\nRead full page 0 size(%lu)", FLASH_PAGE_SIZE);
        printbuf(buf, FLASH_PAGE_SIZE);

        if(!check_name(buf)) set_side(f, get_target_flash_side());

        if(buf[hw_flash_addr_side]==hw_flash_side_left) printf("\n\nSide - LEFT\n");
        if(buf[hw_flash_addr_side]==hw_flash_side_right) printf("\n\nSide - RIGHT\n");
    }
}
