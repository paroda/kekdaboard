#include "stdlib.h"
#include "string.h"

#include "rtc_ds3231.h"

#define DS3231_I2C_ADDRESS 0x68

#define DS3231_REG_TIME 0x00
#define DS3231_REG_TEMP 0x11

#define YEAR_OFFSET 2000

rtc_t* rtc_default_instance;

char* rtc_week[] = {"XXX", "Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

rtc_t* rtc_create(i2c_inst_t* i2c, uint8_t gpio_SCL, uint8_t gpio_SDA) {
    rtc_t* rtc = (rtc_t*) malloc(sizeof(rtc_t));
    rtc->i2c = i2c;
    rtc->gpio_SCL = gpio_SCL;
    rtc->gpio_SDA = gpio_SDA;

    i2c_init(rtc->i2c, 100*1000);
	gpio_set_function(rtc->gpio_SCL, GPIO_FUNC_I2C);
	gpio_set_function(rtc->gpio_SDA, GPIO_FUNC_I2C);
	gpio_pull_up(rtc->gpio_SCL);
	gpio_pull_up(rtc->gpio_SDA);

    return rtc;
}

void rtc_free(rtc_t* rtc) {
    if(rtc_default_instance==rtc) rtc_default_instance = NULL;
    free(rtc);
}

void rtc_set_default_instance(rtc_t* rtc) {
    rtc_default_instance = rtc;
}

static inline uint8_t bcd_decode(uint8_t bcd) {
    return ((bcd>>4)*10) + (bcd & 0x0F);
}

static uint8_t bcd_encode(uint8_t dec) {
    return ((dec/10)<<4) | (dec%10);
}

void rtc_set_time(rtc_t* rtc, const rtc_datetime_t* dt) {
    uint8_t buf[2];
    // second
    buf[0] = 0x00;
    buf[1] = bcd_encode(dt->second);
    i2c_write_blocking(rtc->i2c, DS3231_I2C_ADDRESS, buf, 2, false);
    // minute
    buf[0] = 0x01;
    buf[1] = bcd_encode(dt->minute);
    i2c_write_blocking(rtc->i2c, DS3231_I2C_ADDRESS, buf, 2, false);
    // hour
    buf[0] = 0x02;
    buf[1] = bcd_encode(dt->hour);
    // weekday
    i2c_write_blocking(rtc->i2c, DS3231_I2C_ADDRESS, buf, 2, false);
    buf[0] = 0x03;
    buf[1] = bcd_encode(dt->weekday);
    // date
    i2c_write_blocking(rtc->i2c, DS3231_I2C_ADDRESS, buf, 2, false);
    buf[0] = 0x04;
    buf[1] = bcd_encode(dt->date);
    // century & month
    i2c_write_blocking(rtc->i2c, DS3231_I2C_ADDRESS, buf, 2, false);
    buf[0] = 0x05;
    buf[1] = (((dt->year-YEAR_OFFSET)>=100)?0x80:0) | bcd_encode(dt->month);
    // year
    i2c_write_blocking(rtc->i2c, DS3231_I2C_ADDRESS, buf, 2, false);
    buf[0] = 0x06;
    buf[1] = bcd_encode(dt->year%100);
    i2c_write_blocking(rtc->i2c, DS3231_I2C_ADDRESS, buf, 2, false);
}

void rtc_read_time(rtc_t* rtc, rtc_datetime_t* dt) {
    uint8_t buf[7] = {0x00};
    i2c_write_blocking(rtc->i2c, DS3231_I2C_ADDRESS, buf, 1, true);
    i2c_read_blocking(rtc->i2c, DS3231_I2C_ADDRESS, buf, 7, false);
    dt->second = bcd_decode(buf[0]);
    dt->minute = bcd_decode(buf[1]);
    dt->hour = bcd_decode(buf[2]);
    dt->weekday = bcd_decode(buf[3]);
    dt->date = bcd_decode(buf[4]);
    dt->month = bcd_decode(buf[5] & 0x7F);
    dt->year = bcd_decode(buf[6]) + (100*((buf[5]&0x80)?1:0)) + YEAR_OFFSET;
}

int8_t rtc_get_temperature(rtc_t* rtc) {
    uint8_t reg = 0x11, val;
    i2c_write_blocking(rtc->i2c, DS3231_I2C_ADDRESS, &reg, 1, true);
    i2c_read_blocking(rtc->i2c, DS3231_I2C_ADDRESS, &val, 1, false);
    return (int8_t) val;
}
