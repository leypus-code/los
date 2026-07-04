#include <stdarg.h>
#include <stdint.h>
#include "../include/kprintf.h"
#include "../include/terminal.h"

static void print_string(const char *s) {
    if (!s) {
        terminal_writestring("(null)");
        return;
    }

    terminal_writestring(s);
}

static void print_uint(uint32_t value) {
    char buffer[16];
    int i = 0;

    if (value == 0) {
        terminal_putchar('0');
        return;
    }

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        terminal_putchar(buffer[--i]);
    }
}

static void print_int(int32_t value) {
    if (value < 0) {
        terminal_putchar('-');
        print_uint((uint32_t)(-value));
    } else {
        print_uint((uint32_t)value);
    }
}

static void print_hex(uint32_t value) {
    const char *hex = "0123456789ABCDEF";
    terminal_writestring("0x");

    for (int i = 28; i >= 0; i -= 4) {
        terminal_putchar(hex[(value >> i) & 0xF]);
    }
}

void kprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    for (size_t i = 0; format[i] != '\0'; i++) {
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            continue;
        }

        i++;

        switch (format[i]) {
            case 's':
                print_string(va_arg(args, const char *));
                break;
            case 'd':
                print_int(va_arg(args, int32_t));
                break;
            case 'u':
                print_uint(va_arg(args, uint32_t));
                break;
            case 'x':
                print_hex(va_arg(args, uint32_t));
                break;
            case 'c':
                terminal_putchar((char)va_arg(args, int));
                break;
            case '%':
                terminal_putchar('%');
                break;
            default:
                terminal_putchar('%');
                terminal_putchar(format[i]);
                break;
        }
    }

    va_end(args);
}
