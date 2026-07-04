#ifndef AIOS_TERMINAL_H
#define AIOS_TERMINAL_H

#include <stddef.h>
#include <stdint.h>

#define VGA_BLACK         0
#define VGA_BLUE          1
#define VGA_GREEN         2
#define VGA_CYAN          3
#define VGA_RED           4
#define VGA_MAGENTA       5
#define VGA_BROWN         6
#define VGA_LIGHT_GREY    7
#define VGA_DARK_GREY     8
#define VGA_LIGHT_BLUE    9
#define VGA_LIGHT_GREEN   10
#define VGA_LIGHT_CYAN    11
#define VGA_LIGHT_RED     12
#define VGA_LIGHT_MAGENTA 13
#define VGA_YELLOW        14
#define VGA_WHITE         15

uint8_t terminal_make_color(uint8_t fg, uint8_t bg);

void terminal_initialize(void);
void terminal_set_color(uint8_t color);
void terminal_putchar(char c);
void terminal_write(const char *data, size_t size);
void terminal_writestring(const char *data);
void terminal_enable_cursor(void);
void terminal_update_cursor(void);

void terminal_set_output_muted(int muted);
int terminal_is_output_muted(void);

#endif

void terminal_putentry_at(char c, uint8_t color, size_t x, size_t y);
void terminal_move_cursor(size_t x, size_t y);
size_t terminal_get_cursor_x(void);
size_t terminal_get_cursor_y(void);

void terminal_save_screen(void);
void terminal_restore_screen(void);

void terminal_scroll_up_view(void);
void terminal_scroll_down_view(void);
void terminal_scroll_home(void);
void terminal_scroll_end(void);

void terminal_copy_screen_to(uint16_t *dst);
void terminal_copy_screen_from(const uint16_t *src);
