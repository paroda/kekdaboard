#include "ff15/ff.h"
#include "../../kbd/rtc_ds3231.h"

// Called by FatFs:
DWORD get_fattime(void) {
    rtc_t* rtc = rtc_default_instance;
    if(!rtc) return 0;

    rtc_datetime_t t;
    rtc_read_time(rtc, &t);

    DWORD fattime = 0;
    // bit31:25
    // Year origin from the 1980 (0..127, e.g. 37 for 2017)
    uint8_t yr = t.year - 1980;
    fattime |= (0b01111111 & yr) << 25;
    // bit24:21
    // Month (1..12)
    uint8_t mo = t.month;
    fattime |= (0b00001111 & mo) << 21;
    // bit20:16
    // Day of the month (1..31)
    uint8_t da = t.date;
    fattime |= (0b00011111 & da) << 16;
    // bit15:11
    // Hour (0..23)
    uint8_t hr = t.hour;
    fattime |= (0b00011111 & hr) << 11;
    // bit10:5
    // Minute (0..59)
    uint8_t mi = t.minute;
    fattime |= (0b00111111 & mi) << 5;
    // bit4:0
    // Second / 2 (0..29, e.g. 25 for 50)
    uint8_t sd = t.second / 2;
    fattime |= (0b00011111 & sd);
    return fattime;
}
