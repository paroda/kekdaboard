#ifndef _RTC_MODEL_H
#define _RTC_MODEL_H

#include <stdint.h>

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t date;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} rtc_datetime_t;

#endif
