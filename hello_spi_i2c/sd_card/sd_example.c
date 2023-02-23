#include <stdio.h>

#include "f_util.h"
#include "ff15/ff.h"
#include "pico/stdlib.h"
#include "sd_card.h"
#include "../../kbd/rtc_ds3231.h"

int main() {
    stdio_init_all();

    sleep_ms(5000);
    printf("\nHello from PICO, Booting up..");

    // master SPI
    uint8_t gpio_CLK = 2; // GP2 SPI0 SCK
    uint8_t gpio_MOSI = 3; // GP3 SPI0 TX
    uint8_t gpio_MISO = 4; // GP4 SPI0 RX

    // SD card
    uint8_t gpio_CS_sd_card  = 7; // GP7

    // RTC
    uint8_t gpio_SCL = 27; // GP27 I2C1 SCL
    uint8_t gpio_SDA = 26; // GP26 I2C1 SDA

    /* uint8_t gpio_BTN = 15; // GP15 action button */

    master_spi_t* m_spi = master_spi_create(spi0, 1, gpio_MOSI, gpio_MISO, gpio_CLK);

    sd_card_t* sd = sd_create(m_spi, gpio_CS_sd_card);
    disk_register_sd_card(sd);

    rtc_t* rtc = rtc_create(i2c1, gpio_SCL, gpio_SDA);
    rtc_set_default_instance(rtc);

    sd_print(sd);

    if(sd->status==0) {
        printf("\nSD Card OK (capacity: %llu MB)", sd->sectors/2048U);
    } else {
        printf("\nUnable to access SD Card (status: %d)", sd->status);
        goto DONE;
    }

    FRESULT fr = f_mount(&sd->fatfs, sd->pcName, 1); // 1=mount immediataly
    if(FR_OK != fr) {
        printf("\nf_mount error: %s (%d)", FRESULT_str(fr), fr);
        goto DONE;
    }

    sd_print(sd);

    BYTE buff[101];
    UINT n;
    FIL fil;
    const char* const filename = "filename.txt";

    // WRITE
    fr = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if(FR_OK != fr && FR_EXIST != fr) {
        printf("\nf_open(%s) error: %s (%d)", filename, FRESULT_str(fr), fr);
        goto DONE;
    }

    uint32_t t = time_us_32();
    if(f_printf(&fil, "Hello, world! %lu\n", t) < 0) {
        printf("f_printf failed\n");
    }

    fr = f_close(&fil);
    if(FR_OK != fr) {
        printf("\nf_close error: %s (%d)", FRESULT_str(fr), fr);
    } else {
        printf("\nclosed the file");
    }

    // READ
    fr = f_open(&fil, filename, FA_READ);
    if(FR_OK != fr && FR_EXIST != fr) {
        printf("\nf_open(%s) error: %s (%d)", filename, FRESULT_str(fr), fr);
        goto DONE;
    }

    fr = f_read(&fil, buff, 100U, &n);
    if(FR_OK != fr) {
        printf("\nf_read error: %s (%d)", FRESULT_str(fr), fr);
        goto DONE;
    } else {
        buff[n] = 0;
        printf("\nf_read size: %lu content: %s", n, buff);
    }

    fr = f_close(&fil);
    if(FR_OK != fr) {
        printf("\nf_close error: %s (%d)", FRESULT_str(fr), fr);
    } else {
        printf("\nclosed the file");
    }

    f_unmount(sd->pcName);

    sd_print(sd);

 DONE:
    printf("\nGoodbye, world!");
    printf("\nTHE END\n");
    fflush(stdout);

    while(true) {
        sleep_ms(1000);
        printf(".");
    }
}
