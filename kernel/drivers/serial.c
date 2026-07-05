#include "../include/serial.h"
#include "../include/io.h"
#include <stdint.h>

#define COM1 0x3F8

static int serial_ready = 0;

static int serial_received(void) {
    return inb(COM1 + 5) & 1;
}

static int serial_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

void serial_initialize(void) {
    /*
     * COM1 init: 38400 baud, 8N1, FIFO enabled.
     * IRQ disabled: we use polling, reliable under QEMU.
     */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);

    serial_ready = 1;
}

int serial_read_char(void) {
    if (!serial_ready) {
        return -1;
    }

    if (!serial_received()) {
        return -1;
    }

    return inb(COM1);
}

void serial_write_char(char c) {
    if (!serial_ready) {
        return;
    }

    for (int i = 0; i < 100000; i++) {
        if (serial_transmit_empty()) {
            outb(COM1, (uint8_t)c);
            return;
        }
    }
}

void serial_write_string(const char *s) {
    if (!s) {
        return;
    }

    while (*s) {
        serial_write_char(*s++);
    }
}


int serial_is_ready(void) {
    return serial_ready;
}

int serial_read_char_nonblocking(char *out) {
    int ch;

    if (!out) {
        return 0;
    }

    ch = serial_read_char();

    if (ch < 0) {
        return 0;
    }

    *out = (char)ch;
    return 1;
}
