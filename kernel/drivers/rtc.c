#include "../include/rtc.h"
#include "../include/io.h"

static uint8_t cmos_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static uint8_t bcd_to_binary(uint8_t value) {
    return (value & 0x0F) + ((value / 16) * 10);
}

static uint8_t rtc_update_in_progress(void) {
    outb(0x70, 0x0A);
    return inb(0x71) & 0x80;
}

void rtc_read_time(rtc_time_t *time) {
    while (rtc_update_in_progress()) {
        // wait
    }

    time->second = cmos_read(0x00);
    time->minute = cmos_read(0x02);
    time->hour   = cmos_read(0x04);
    time->day    = cmos_read(0x07);
    time->month  = cmos_read(0x08);
    time->year   = cmos_read(0x09);

    uint8_t reg_b = cmos_read(0x0B);

    if (!(reg_b & 0x04)) {
        time->second = bcd_to_binary(time->second);
        time->minute = bcd_to_binary(time->minute);
        time->hour   = bcd_to_binary(time->hour & 0x7F);
        time->day    = bcd_to_binary(time->day);
        time->month  = bcd_to_binary(time->month);
        time->year   = bcd_to_binary(time->year);
    }
}
