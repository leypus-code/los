#include "../include/eventlog.h"
#include <stdint.h>
#include "../include/kprintf.h"

#define EVENTLOG_MAX 64
#define EVENTLOG_LINE 80

static char eventlog[EVENTLOG_MAX][EVENTLOG_LINE];
static int eventlog_count = 0;

static void copy_line(char *dst, const char *src) {
    int i = 0;

    while (src && src[i] && i < EVENTLOG_LINE - 1) {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
}

void eventlog_initialize(void) {
    eventlog_count = 0;
    eventlog_add("eventlog initialized");
}

void eventlog_add(const char *message) {
    if (eventlog_count >= EVENTLOG_MAX) {
        for (int i = 1; i < EVENTLOG_MAX; i++) {
            copy_line(eventlog[i - 1], eventlog[i]);
        }

        eventlog_count = EVENTLOG_MAX - 1;
    }

    copy_line(eventlog[eventlog_count], message);
    eventlog_count++;
}

void eventlog_print(void) {
    for (int i = 0; i < eventlog_count; i++) {
        kprintf("[%u] %s\n", (uint32_t)i, eventlog[i]);
    }
}
