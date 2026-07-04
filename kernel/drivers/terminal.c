#include "../include/terminal.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t *terminal_buffer = (uint16_t *)0xB8000;

uint8_t terminal_make_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

static uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | (uint16_t)color << 8;
}

void terminal_set_color(uint8_t color) {
    terminal_color = color;
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = terminal_make_color(VGA_WHITE, VGA_BLUE);

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
        }
    }
}

static void terminal_newline(void) {
    terminal_column = 0;

    if (++terminal_row == VGA_HEIGHT) {
        terminal_row = VGA_HEIGHT - 1;

        for (size_t y = 1; y < VGA_HEIGHT; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                terminal_buffer[(y - 1) * VGA_WIDTH + x] =
                    terminal_buffer[y * VGA_WIDTH + x];
            }
        }

        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
                vga_entry(' ', terminal_color);
        }
    }
}

static int terminal_output_muted = 0;

void terminal_putchar(char c) {
    if (terminal_output_muted) {
        return;
    }

    if (c == '\n') {
        terminal_newline();
        terminal_update_cursor();
        return;
    }

    if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
            terminal_buffer[terminal_row * VGA_WIDTH + terminal_column] =
                vga_entry(' ', terminal_color);
            terminal_update_cursor();
        }
        return;
    }

    terminal_buffer[terminal_row * VGA_WIDTH + terminal_column] =
        vga_entry(c, terminal_color);

    if (++terminal_column == VGA_WIDTH) {
        terminal_newline();
    }

    terminal_update_cursor();
}

void terminal_write(const char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

void terminal_writestring(const char *data) {
    size_t len = 0;
    while (data[len]) {
        len++;
    }

    terminal_write(data, len);
}

#include "../include/io.h"

void terminal_enable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 14);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

void terminal_update_cursor(void) {
    uint16_t pos = terminal_row * VGA_WIDTH + terminal_column;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_putentry_at(char c, uint8_t color, size_t x, size_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        return;
    }

    terminal_buffer[y * VGA_WIDTH + x] = vga_entry(c, color);
}

void terminal_move_cursor(size_t x, size_t y) {
    terminal_column = x;
    terminal_row = y;
    terminal_update_cursor();
}

static uint16_t terminal_saved_buffer[80 * 25];
static size_t terminal_saved_row = 0;
static size_t terminal_saved_column = 0;
static int terminal_has_saved_screen = 0;

void terminal_save_screen(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        terminal_saved_buffer[i] = terminal_buffer[i];
    }

    terminal_saved_row = terminal_row;
    terminal_saved_column = terminal_column;
    terminal_has_saved_screen = 1;
}

void terminal_restore_screen(void) {
    if (!terminal_has_saved_screen) {
        return;
    }

    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        terminal_buffer[i] = terminal_saved_buffer[i];
    }

    terminal_row = terminal_saved_row;
    terminal_column = terminal_saved_column;
    terminal_update_cursor();
}

#define SCROLLBACK_LINES 200

static uint16_t terminal_history[SCROLLBACK_LINES][80];
static size_t terminal_history_count = 0;
static size_t terminal_view_offset = 0;

__attribute__((unused)) static void terminal_history_capture_screen(void) {
    if (terminal_history_count >= SCROLLBACK_LINES) {
        for (size_t y = 1; y < SCROLLBACK_LINES; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                terminal_history[y - 1][x] = terminal_history[y][x];
            }
        }
        terminal_history_count = SCROLLBACK_LINES - 1;
    }

    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_history[terminal_history_count][x] =
            terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x];
    }

    terminal_history_count++;
}

static void terminal_render_history_view(void) {
    if (terminal_view_offset == 0) {
        return;
    }

    terminal_initialize();

    size_t available = terminal_history_count < VGA_HEIGHT ? terminal_history_count : VGA_HEIGHT;
    size_t start = 0;

    if (terminal_history_count > terminal_view_offset + available) {
        start = terminal_history_count - terminal_view_offset - available;
    }

    for (size_t y = 0; y < available; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = terminal_history[start + y][x];
        }
    }

    terminal_move_cursor(0, 24);
}

void terminal_scroll_up_view(void) {
    if (terminal_history_count == 0) {
        return;
    }

    if (terminal_view_offset + 1 < terminal_history_count) {
        terminal_view_offset++;
    }

    terminal_render_history_view();
}

void terminal_scroll_down_view(void) {
    if (terminal_view_offset > 0) {
        terminal_view_offset--;
    }

    if (terminal_view_offset == 0) {
        return;
    }

    terminal_render_history_view();
}

void terminal_scroll_home(void) {
    if (terminal_history_count == 0) {
        return;
    }

    terminal_view_offset = terminal_history_count - 1;
    terminal_render_history_view();
}

void terminal_scroll_end(void) {
    terminal_view_offset = 0;
}

void terminal_copy_screen_to(uint16_t *dst) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        dst[i] = terminal_buffer[i];
    }
}

void terminal_copy_screen_from(const uint16_t *src) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        terminal_buffer[i] = src[i];
    }

    terminal_update_cursor();
}


size_t terminal_get_cursor_x(void) {
    return terminal_column;
}

size_t terminal_get_cursor_y(void) {
    return terminal_row;
}


void terminal_set_output_muted(int muted) {
    terminal_output_muted = muted ? 1 : 0;
}

int terminal_is_output_muted(void) {
    return terminal_output_muted;
}
