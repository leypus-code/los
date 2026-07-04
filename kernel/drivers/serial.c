#include "../include/serial.h"
#include "../include/io.h"

#define COM1 0x3F8

static int serial_ready = 0;

static int serial_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

static int serial_received(void) {
    return inb(COM1 + 5) & 1;
}

void serial_initialize(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);

    serial_ready = 1;
}

int serial_is_ready(void) {
    return serial_ready;
}

void serial_write_char(char c) {
    if (!serial_ready) return;

    while (!serial_transmit_empty()) {
    }

    outb(COM1, (unsigned char)c);
}

void serial_write_string(const char *s) {
    if (!s) return;

    for (int i = 0; s[i]; i++) {
        serial_write_char(s[i]);
    }
}

int serial_read_char_nonblocking(char *out) {
    if (!serial_ready || !out) return 0;

    if (!serial_received()) {
        return 0;
    }

    *out = (char)inb(COM1);
    return 1;
}
