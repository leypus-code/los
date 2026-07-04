#include "../include/wm.h"
#include "../include/terminal.h"
#include "../include/keycodes.h"
#include "../include/ui.h"
#include "../include/shell.h"
#include "../include/kprintf.h"

static int wm_active = 0;
static int focused_window = 0;

static uint8_t bg;
static uint8_t border;
static uint8_t title;
static uint8_t text;
static uint8_t footer;

static void wm_colors(void) {
    bg = terminal_make_color(VGA_WHITE, VGA_BLUE);
    border = terminal_make_color(VGA_LIGHT_CYAN, VGA_BLUE);
    title = terminal_make_color(VGA_YELLOW, VGA_BLUE);
    text = terminal_make_color(VGA_WHITE, VGA_BLUE);
    footer = terminal_make_color(VGA_LIGHT_CYAN, VGA_BLACK);
}

static void put(size_t x, size_t y, char c, uint8_t color) {
    terminal_putentry_at(c, color, x, y);
}

static void print_at(size_t x, size_t y, const char *s, uint8_t color) {
    for (size_t i = 0; s[i] && x + i < 80; i++) {
        put(x + i, y, s[i], color);
    }
}

static void clear_screen(void) {
    wm_colors();

    for (size_t y = 0; y < 25; y++) {
        for (size_t x = 0; x < 80; x++) {
            put(x, y, ' ', bg);
        }
    }
}

static void draw_box(size_t x, size_t y, size_t w, size_t h, const char *caption, int active) {
    uint8_t b = active ? terminal_make_color(VGA_YELLOW, VGA_BLUE) : border;

    put(x, y, '+', b);
    put(x + w - 1, y, '+', b);
    put(x, y + h - 1, '+', b);
    put(x + w - 1, y + h - 1, '+', b);

    for (size_t i = 1; i < w - 1; i++) {
        put(x + i, y, '-', b);
        put(x + i, y + h - 1, '-', b);
    }

    for (size_t i = 1; i < h - 1; i++) {
        put(x, y + i, '|', b);
        put(x + w - 1, y + i, '|', b);
    }

    print_at(x + 3, y, caption, title);
}

static void wm_draw(void) {
    clear_screen();

    print_at(2, 0, "LOS Window Manager", title);
    print_at(55, 0, "Desktop v0.1", title);

    draw_box(2, 2, 36, 12, " Shell ", focused_window == 0);
    print_at(5, 5, "Shell window placeholder", text);
    print_at(5, 7, "Later: embedded shell app", text);

    draw_box(42, 5, 34, 12, " Apps ", focused_window == 1);
    print_at(45, 8, "Norton", text);
    print_at(45, 9, "Editor", text);
    print_at(45, 10, "Calculator", text);

    for (size_t x = 0; x < 80; x++) {
        put(x, 24, ' ', footer);
    }

    print_at(0, 24, "Tab Switch   Enter Open   Q Exit WM", footer);

    terminal_move_cursor(0, 24);
}

void wm_initialize(void) {
    wm_active = 0;
    focused_window = 0;
}

void wm_enter(void) {
    terminal_save_screen();
    wm_active = 1;
    focused_window = 0;
    ui_enter(UI_WM);
    wm_draw();
}

void wm_handle_key(int key) {
    if (key == 'q' || key == KEY_ESCAPE || key == KEY_F10) {
        wm_active = 0;
        ui_exit_to_shell();
        return;
    }

    if (key == KEY_TAB || key == KEY_ARROW_LEFT || key == KEY_ARROW_RIGHT) {
        focused_window = focused_window ? 0 : 1;
        wm_draw();
        return;
    }

    if (key == KEY_ENTER) {
        wm_draw();

        if (focused_window == 0) {
            print_at(5, 9, "Shell activation coming soon", text);
        } else {
            print_at(45, 12, "App launcher coming soon", text);
        }

        return;
    }
}

void wm_status(void) {
    kprintf("Window Manager: %s\n", wm_active ? "active" : "inactive");
    kprintf("Focused window: %u\n", (uint32_t)focused_window);
}
