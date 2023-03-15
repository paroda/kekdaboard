#ifndef __RTC_DS3231_H
#define __RTC_DS3231_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "rtc_model.h"

typedef struct {
    i2c_inst_t* i2c;
    uint8_t gpio_SCL;
    uint8_t gpio_SDA;
} rtc_t;

extern rtc_t* rtc_default_instance;

extern char* rtc_week[];

rtc_t* rtc_create(i2c_inst_t* i2c, uint8_t gpio_SCL, uint8_t gpio_SDA);

void rtc_free(rtc_t* rtc);

void rtc_set_default_instance(rtc_t* rtc);

void rtc_set_time(rtc_t* rtc, const rtc_datetime_t* dt);

void rtc_read_time(rtc_t* rtc, rtc_datetime_t* dt);

int8_t rtc_get_temperature(rtc_t* rtc);

#endif
