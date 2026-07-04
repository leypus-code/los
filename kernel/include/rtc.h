#ifndef LOS_RTC_H
#define LOS_RTC_H

#include <stdint.h>

typedef struct rtc_time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} rtc_time_t;

void rtc_read_time(rtc_time_t *time);

#endif
