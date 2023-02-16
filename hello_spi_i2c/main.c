#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "master_spi.h"
#include "flash_w25qxx.h"
#include "rtc_ds3231.h"

#include "blink_led.h"

void init_btn_action(uint8_t gpio_BTN) {
    gpio_init(gpio_BTN);
    gpio_set_dir(gpio_BTN, GPIO_IN);
}

bool scan_btn_action(uint8_t gpio_BTN) {
    static bool press = false;
    bool action = false;
    if(gpio_get(gpio_BTN)) {
        if(!press) action = true;
        press = true;
    } else {
        press = false;
    }
    return action;
}

void printbuf(uint8_t* buf, size_t len) {
    for(int i=0; i<len; i++) {
        if(i%16 == 15)
            printf("%02x\n", buf[i]);
        else if(i%4 == 3)
            printf("%02x, ", buf[i]);
        else
            printf("%02x ", buf[i]);
    }
}

void task_flash_action1(flash_t* f) {
    printf("\nTask Flash Action 1");
    uint64_t start_us, end_us, dt;

    start_us = time_us_64();
    flash_sector_erase(f, 0);
    end_us = time_us_64();
    dt = end_us - start_us;
    printf("\nErase sector (%llu us)\n", dt); // ~ 90 ms

    uint8_t buf[FLASH_PAGE_SIZE];
    memset(buf, 0, FLASH_PAGE_SIZE);
    flash_read(f, 0, buf, FLASH_PAGE_SIZE);
    printf("\nRead\n");
    printbuf(buf, FLASH_PAGE_SIZE);

    buf[0]   = 0x01;
    buf[128] = 0x02;
    buf[251] = 0x81;
    buf[255] = 0xAB;
    start_us = time_us_64();
    flash_page_program(f, 0, buf, FLASH_PAGE_SIZE);
    end_us = time_us_64();
    dt = end_us - start_us;
    printf("\nProgram full page (%llu us)\n", dt); // ~ 300 us

    memset(buf, 0, FLASH_PAGE_SIZE);
    flash_read(f, 0, buf, FLASH_PAGE_SIZE);
    printf("\nRead\n");
    printbuf(buf, FLASH_PAGE_SIZE);
}

void task_flash_action2(flash_t* f) {
    printf("\nTask Flash Action 2");
    uint64_t start_us, end_us, dt;

    uint8_t buf[FLASH_PAGE_SIZE];

    start_us = time_us_64();
    flash_sector_erase(f, 0);
    end_us = time_us_64();
    dt = end_us - start_us;
    printf("\nErase sector (%llu us)\n", dt);

    buf[0] = 0; buf[1] = 1; buf[2] = 2; buf[3] = 3;
    start_us = time_us_64();
    flash_page_program(f, 0, buf, 4);
    end_us = time_us_64();
    dt = end_us - start_us;
    printf("\nProgram 4-bytes (%llu us)\n", dt);

    buf[0] = 0x80; buf[1] = 0x81; buf[2] = 0x82; buf[3] = 0x83;
    flash_page_program(f, 4, buf, 4);
    flash_page_program(f, 128, buf, 4);

    buf[0] = 0xF0; buf[1] = 0xF1; buf[2] = 0xF2; buf[3] = 0xF3;
    flash_page_program(f, 8, buf, 4);
    flash_page_program(f, 252, buf, 4);

    memset(buf, 0, FLASH_PAGE_SIZE);
    flash_read(f, 0, buf, FLASH_PAGE_SIZE);
    printf("\nRead\n");
    printbuf(buf, FLASH_PAGE_SIZE);

    // once set to 0, program cannot set it to 1
    // program only sets 1 to 0
    // to set back to 1, only option is to erase

    // try setting all 0xff
    memset(buf, 0xff, FLASH_PAGE_SIZE);
    start_us = time_us_64();
    flash_page_program(f, 0, buf, FLASH_PAGE_SIZE);
    end_us = time_us_64();
    dt = end_us - start_us;
    printf("\nProgram full page 0xff (%llu us)\n", dt);

    buf[0] = 0xF0; buf[1] = 0xF1; buf[2] = 0xF2; buf[3] = 0xF3;
    flash_page_program(f, 12, buf, 4);
    flash_page_program(f, 244, buf, 4);

    memset(buf, 0, FLASH_PAGE_SIZE);
    flash_read(f, 0, buf, FLASH_PAGE_SIZE);
    printf("\nRead\n");
    printbuf(buf, FLASH_PAGE_SIZE);

    // try setting all 0x00
    memset(buf, 0x00, FLASH_PAGE_SIZE);
    start_us = time_us_64();
    flash_page_program(f, 0, buf, FLASH_PAGE_SIZE);
    end_us = time_us_64();
    dt = end_us - start_us;
    printf("\nProgram full page 0x00 (%llu us)\n", dt);

    buf[0] = 0xF0; buf[1] = 0xF1; buf[2] = 0xF2; buf[3] = 0xF3;
    flash_page_program(f, 16, buf, 4);
    flash_page_program(f, 240, buf, 4);

    memset(buf, 0, FLASH_PAGE_SIZE);
    flash_read(f, 0, buf, FLASH_PAGE_SIZE);
    printf("\nRead\n");
    printbuf(buf, FLASH_PAGE_SIZE);
}

void task_flash_action3(flash_t* f) {
    printf("\nTask Flash Action 3");
    uint64_t start_us, end_us, dt;

    uint8_t buf[FLASH_PAGE_SIZE];

    // try 32K block erase

    start_us = time_us_64();
    flash_block_erase_32K(f, 64*1024);
    end_us = time_us_64();
    dt = end_us - start_us;
    printf("\nErase block 32K (%llu us)\n", dt);

    buf[0] = 0; buf[1] = 1; buf[2] = 2; buf[3] = 3;
    flash_page_program(f, 0, buf, 4);

    memset(buf, 0, FLASH_PAGE_SIZE);
    flash_read(f, 0, buf, 4);
    printf("\nRead 4 bytes: %u %u %u %u", buf[0], buf[1], buf[2], buf[3]);

    // try 64K block erase

    start_us = time_us_64();
    flash_block_erase_64K(f, 2*64*1024);
    end_us = time_us_64();
    dt = end_us - start_us;
    printf("\nErase block 64K (%llu us)\n", dt);

    buf[0] = 0x80; buf[1] = 0x81; buf[2] = 0x82; buf[3] = 0x83;
    flash_page_program(f, 0, buf, 4);

    memset(buf, 0, FLASH_PAGE_SIZE);
    flash_read(f, 0, buf, 4);
    printf("\nRead 4 bytes: %u %u %u %u", buf[0], buf[1], buf[2], buf[3]);

    // try chip erase

    start_us = time_us_64();
    flash_chip_erase_all(f);
    end_us = time_us_64();
    dt = end_us - start_us;
    printf("\nErase chip whole (%llu us)\n", dt);

    buf[0] = 0x80; buf[1] = 0x81; buf[2] = 0x82; buf[3] = 0x83;
    flash_page_program(f, 0, buf, 4);

    memset(buf, 0, FLASH_PAGE_SIZE);
    flash_read(f, 0, buf, 4);
    printf("\nRead 4 bytes: %u %u %u %u", buf[0], buf[1], buf[2], buf[3]);

}

void task_flash(flash_t* f, bool* action) {
    static uint32_t interval_ms = 5000;
    static uint32_t start_ms = 0;
    if(start_ms == 0) start_ms = board_millis();

    uint8_t buf[FLASH_PAGE_SIZE];
    memset(buf, 0, FLASH_PAGE_SIZE);
    uint32_t addr = 0;
    uint64_t start_us, end_us, dt;

    if(board_millis() - start_ms > interval_ms) {
        start_ms += interval_ms;

        uint32_t id = flash_get_id(f);
        printf("\n\nFlash device ID: %#010x\n", id);

        uint32_t baud = spi_get_baudrate(f->m_spi->spi);
        printf("\nBaud: %lu", baud);

        start_us = time_us_64();
        flash_read(f, addr, buf, FLASH_PAGE_SIZE);
        end_us = time_us_64();
        dt = end_us - start_us;
        printf("\nRead full page (%llu us): addr(%lu) size(%lu)\n", dt, addr, FLASH_PAGE_SIZE);
        printbuf(buf, FLASH_PAGE_SIZE);

        memset(buf, 0, FLASH_PAGE_SIZE);
        start_us = time_us_64();
        flash_read(f, addr, buf, 4);
        end_us = time_us_64();
        dt = end_us -start_us;
        printf("\nRead 4-bytes (%llu us): %u %u %u %u", dt, buf[0], buf[1], buf[2], buf[3]);

        if(*action) {
            task_flash_action2(f);
            *action = false;
        }
    }

}

void print_datetime(rtc_datetime_t* dt) {
    printf("%d-%02d-%02d (%d:%s) %02d:%02d:%02d",
           dt->year, dt->month, dt->date,
           dt->weekday, rtc_week[dt->weekday],
           dt->hour, dt->minute, dt->second);
}

void task_rtc(rtc_t* rtc, bool* action) {
    static uint32_t interval_ms = 5000;
    static uint32_t start_ms = 0;
    if(start_ms == 0) start_ms = board_millis();

    rtc_datetime_t dt;
    int8_t temp;
    if(board_millis() - start_ms > interval_ms) {
        start_ms += interval_ms;
        printf("\n\nRTC Trials");

        rtc_read_time(rtc, &dt);
        printf("\nRTC time: "); print_datetime(&dt);
        temp = rtc_get_temperature(rtc);
        printf("\nRTC temperature: %d", temp);

        if(*action) {
            // TODO: action
            dt.year = 2023;
            dt.month = 2;
            dt.date = 16;
            dt.weekday = 5;
            dt.hour = 13;
            dt.minute = 20;
            dt.second = 0;
            rtc_set_time(rtc, &dt);
            *action = false;
        }
    }
}

int main(void) {
    stdio_init_all();

    sleep_ms(5000);
    printf("\nHello from PICO. Booting up..");

    // master SPI
    uint8_t gpio_CLK = 2; // GP2 SPI0 SCK
    uint8_t gpio_MOSI = 3; // GP3 SPI0 TX
    uint8_t gpio_MISO = 4; // GP4 SPI0 RX

    // flash
    uint8_t gpio_CS_w25qxx  = 5; // GP5

    // SD card
    uint8_t gpio_CS_sd_card  = 7; // GP7

    // RTC
    uint8_t gpio_SCL = 27; // GP27 I2C1 SCL
    uint8_t gpio_SDA = 26; // GP26 I2C1 SDA

    uint8_t gpio_BTN = 15; // GP15 action button
    init_btn_action(gpio_BTN);

    master_spi_t* m_spi = master_spi_create(spi0, 2, gpio_MOSI, gpio_MISO, gpio_CLK);

    flash_t* f = flash_create(m_spi, gpio_CS_w25qxx);

    rtc_t* r = rtc_create(i2c1, gpio_SCL, gpio_SDA);

    while(true) {
        sleep_ms(1);
        led_blinking_task();

        static bool action_flash = false;
        static bool action_rtc = false;
        if(scan_btn_action(gpio_BTN)) {
            action_rtc = true;
        }

        static uint32_t start_ms = 0;
        if(start_ms == 0) start_ms = board_millis();

        if(board_millis() - start_ms > 1000) {
            spi_set_baudrate(m_spi->spi, 125*1000*1000);
            start_ms += 1000;

            printf("\nHello again from PICO. still beating..");
            //print_master_spi(m_spi);

            task_flash(f, &action_flash);

            task_rtc(r, &action_rtc);
        }

        tight_loop_contents();
    }
}
