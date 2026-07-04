#include "../include/theme.h"
#include "../include/terminal.h"
#include "../include/kprintf.h"
#include "../include/version.h"
#include "../include/string.h"

typedef struct los_theme {
    const char *name;
    const char *label;

    uint8_t fg;
    uint8_t bg;
    uint8_t title_fg;
    uint8_t accent_fg;
    uint8_t muted_fg;
    uint8_t error_fg;
    uint8_t ok_fg;
    uint8_t info_fg;
    uint8_t selected_fg;
    uint8_t selected_bg;
    uint8_t dialog_fg;
    uint8_t dialog_bg;
} los_theme_t;

/*
 * VGA text mode has only 16 colors.
 * So themes are curated as high-contrast fg/bg/accent combinations.
 */
static los_theme_t themes[] = {
    { "terminal",   "Default minimal black",       VGA_LIGHT_GREY,    VGA_BLACK,      VGA_WHITE,         VGA_LIGHT_GREY,    VGA_DARK_GREY,     VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_BLACK,         VGA_WHITE,         VGA_BLACK,         VGA_LIGHT_GREY },
    { "classic",    "Classic LOS blue",            VGA_WHITE,         VGA_BLUE,       VGA_YELLOW,        VGA_LIGHT_CYAN,    VGA_LIGHT_GREY,    VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_BLACK,         VGA_LIGHT_CYAN,    VGA_BLACK,         VGA_LIGHT_GREY },
    { "matrix",     "Green hacker black",          VGA_LIGHT_GREEN,   VGA_BLACK,      VGA_WHITE,         VGA_GREEN,         VGA_GREEN,         VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_LIGHT_CYAN,    VGA_BLACK,         VGA_LIGHT_GREEN,   VGA_BLACK,         VGA_GREEN },
    { "amber",      "Amber monochrome",            VGA_BROWN,         VGA_BLACK,      VGA_YELLOW,        VGA_YELLOW,        VGA_LIGHT_GREY,    VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_BLACK,         VGA_YELLOW,        VGA_BLACK,         VGA_BROWN },
    { "ice",        "Ice cyan blue",                VGA_LIGHT_CYAN,    VGA_BLUE,       VGA_WHITE,         VGA_CYAN,          VGA_LIGHT_GREY,    VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_WHITE,         VGA_BLUE,          VGA_WHITE,         VGA_BLUE,          VGA_LIGHT_CYAN },
    { "pink",       "Hot pink magenta",            VGA_WHITE,         VGA_MAGENTA,    VGA_YELLOW,        VGA_LIGHT_MAGENTA, VGA_LIGHT_GREY,    VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_MAGENTA,       VGA_WHITE,         VGA_MAGENTA,       VGA_WHITE },
    { "violet",     "Violet night",                VGA_LIGHT_MAGENTA, VGA_BLACK,      VGA_WHITE,         VGA_MAGENTA,       VGA_DARK_GREY,     VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_LIGHT_CYAN,    VGA_BLACK,         VGA_LIGHT_MAGENTA, VGA_BLACK,         VGA_LIGHT_MAGENTA },
    { "blood",      "Blood red black",             VGA_LIGHT_RED,     VGA_BLACK,      VGA_WHITE,         VGA_RED,           VGA_DARK_GREY,     VGA_WHITE,         VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_BLACK,         VGA_LIGHT_RED,     VGA_BLACK,         VGA_RED },
    { "lava",       "Lava yellow black",           VGA_YELLOW,        VGA_BLACK,      VGA_LIGHT_RED,     VGA_BROWN,         VGA_DARK_GREY,     VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_BLACK,         VGA_YELLOW,        VGA_BLACK,         VGA_BROWN },
    { "forest",     "Forest green dark",           VGA_LIGHT_GREEN,   VGA_GREEN,      VGA_WHITE,         VGA_YELLOW,        VGA_GREEN,         VGA_LIGHT_RED,     VGA_WHITE,         VGA_YELLOW,        VGA_GREEN,         VGA_WHITE,         VGA_GREEN,         VGA_LIGHT_GREEN },
    { "deepsea",    "Deep sea cyan",               VGA_CYAN,          VGA_BLACK,      VGA_LIGHT_CYAN,    VGA_BLUE,          VGA_DARK_GREY,     VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_BLACK,         VGA_CYAN,          VGA_BLACK,         VGA_CYAN },
    { "aqua",       "Aqua white cyan",             VGA_WHITE,         VGA_CYAN,       VGA_BLUE,          VGA_WHITE,         VGA_LIGHT_GREY,    VGA_RED,           VGA_GREEN,         VGA_YELLOW,        VGA_CYAN,          VGA_BLUE,          VGA_CYAN,          VGA_WHITE },
    { "toxic",      "Toxic yellow green",          VGA_YELLOW,        VGA_GREEN,      VGA_WHITE,         VGA_YELLOW,        VGA_LIGHT_GREY,    VGA_LIGHT_RED,     VGA_WHITE,         VGA_YELLOW,        VGA_GREEN,         VGA_WHITE,         VGA_GREEN,         VGA_YELLOW },
    { "paper",      "Paper black gray",            VGA_BLACK,         VGA_LIGHT_GREY, VGA_BLUE,          VGA_DARK_GREY,     VGA_DARK_GREY,     VGA_RED,           VGA_GREEN,         VGA_BROWN,         VGA_LIGHT_GREY,    VGA_BLUE,          VGA_LIGHT_GREY,    VGA_BLACK },
    { "inverse",    "Inverse black white",         VGA_BLACK,         VGA_WHITE,      VGA_BLUE,          VGA_RED,           VGA_DARK_GREY,     VGA_RED,           VGA_GREEN,         VGA_BROWN,         VGA_WHITE,         VGA_BLUE,          VGA_WHITE,         VGA_BLACK },
    { "win95",      "Windows 95 gray",             VGA_BLACK,         VGA_LIGHT_GREY, VGA_BLUE,          VGA_BLUE,          VGA_DARK_GREY,     VGA_RED,           VGA_GREEN,         VGA_BROWN,         VGA_LIGHT_GREY,    VGA_BLUE,          VGA_LIGHT_GREY,    VGA_BLACK },
    { "bios",       "BIOS setup blue",             VGA_WHITE,         VGA_BLUE,       VGA_YELLOW,        VGA_WHITE,         VGA_LIGHT_GREY,    VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_BLUE,          VGA_YELLOW,        VGA_BLUE,          VGA_WHITE },
    { "powershell", "PowerShell blue",             VGA_LIGHT_GREY,    VGA_BLUE,       VGA_WHITE,         VGA_LIGHT_CYAN,    VGA_LIGHT_GREY,    VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_BLUE,          VGA_WHITE,         VGA_BLUE,          VGA_LIGHT_GREY },
    { "ubuntu",     "Ubuntu aubergine",            VGA_LIGHT_GREY,    VGA_RED,        VGA_WHITE,         VGA_YELLOW,        VGA_LIGHT_GREY,    VGA_WHITE,         VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_RED,           VGA_WHITE,         VGA_RED,           VGA_LIGHT_GREY },
    { "raspberry",  "Raspberry red magenta",       VGA_LIGHT_MAGENTA, VGA_RED,        VGA_WHITE,         VGA_YELLOW,        VGA_LIGHT_GREY,    VGA_WHITE,         VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_RED,           VGA_WHITE,         VGA_RED,           VGA_LIGHT_MAGENTA },
    { "royal",      "Royal blue yellow",           VGA_YELLOW,        VGA_BLUE,       VGA_WHITE,         VGA_YELLOW,        VGA_LIGHT_GREY,    VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_LIGHT_CYAN,    VGA_BLUE,          VGA_YELLOW,        VGA_BLUE,          VGA_YELLOW },
    { "steel",      "Steel gray black",            VGA_LIGHT_GREY,    VGA_BLACK,      VGA_LIGHT_CYAN,    VGA_CYAN,          VGA_DARK_GREY,     VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_BLACK,         VGA_LIGHT_CYAN,    VGA_BLACK,         VGA_LIGHT_GREY },
    { "neon",       "Neon cyan magenta",           VGA_LIGHT_CYAN,    VGA_BLACK,      VGA_LIGHT_MAGENTA, VGA_LIGHT_CYAN,    VGA_DARK_GREY,     VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_YELLOW,        VGA_BLACK,         VGA_LIGHT_MAGENTA, VGA_BLACK,         VGA_LIGHT_CYAN },
    { "cyberpunk",  "Cyberpunk yellow pink",       VGA_YELLOW,        VGA_BLACK,      VGA_LIGHT_MAGENTA, VGA_YELLOW,        VGA_DARK_GREY,     VGA_LIGHT_RED,     VGA_LIGHT_GREEN,   VGA_LIGHT_CYAN,    VGA_BLACK,         VGA_YELLOW,        VGA_BLACK,         VGA_YELLOW },
    { "ghost",      "Ghost light gray",            VGA_BLACK,         VGA_LIGHT_GREY, VGA_BLUE,          VGA_BLUE,          VGA_DARK_GREY,     VGA_RED,           VGA_GREEN,         VGA_BROWN,         VGA_LIGHT_GREY,    VGA_BLUE,          VGA_LIGHT_GREY,    VGA_BLACK },
    { "mono-green", "Mono green black",            VGA_GREEN,         VGA_BLACK,      VGA_LIGHT_GREEN,   VGA_GREEN,         VGA_GREEN,         VGA_GREEN,         VGA_LIGHT_GREEN,   VGA_GREEN,         VGA_BLACK,         VGA_LIGHT_GREEN,   VGA_BLACK,         VGA_GREEN },
    { "mono-red",   "Mono red black",              VGA_RED,           VGA_BLACK,      VGA_LIGHT_RED,     VGA_RED,           VGA_RED,           VGA_LIGHT_RED,     VGA_LIGHT_RED,     VGA_RED,           VGA_BLACK,         VGA_LIGHT_RED,     VGA_BLACK,         VGA_RED },
    { "whiteout",   "Whiteout blue on white",      VGA_BLUE,          VGA_WHITE,      VGA_RED,           VGA_BLUE,          VGA_DARK_GREY,     VGA_RED,           VGA_GREEN,         VGA_BROWN,         VGA_WHITE,         VGA_BLUE,          VGA_WHITE,         VGA_BLUE }
};

#define THEME_COUNT ((int)(sizeof(themes) / sizeof(themes[0])))

static int active_theme = -1;

static uint8_t make(uint8_t fg, uint8_t bg) {
    return terminal_make_color(fg, bg);
}

static int theme_find_index(const char *name) {
    if (!name) return -1;

    for (int i = 0; i < THEME_COUNT; i++) {
        if (strcmp(name, themes[i].name) == 0) {
            return i;
        }
    }

    return -1;
}

static void theme_ensure_default(void) {
    if (active_theme >= 0 && active_theme < THEME_COUNT) {
        return;
    }

    int terminal_index = theme_find_index("terminal");
    active_theme = terminal_index >= 0 ? terminal_index : 0;
}

static los_theme_t *current(void) {
    theme_ensure_default();
    return &themes[active_theme];
}

uint8_t theme_color_normal(void) { return make(current()->fg, current()->bg); }
uint8_t theme_color_title(void) { return make(current()->title_fg, current()->bg); }
uint8_t theme_color_footer(void) { return make(current()->accent_fg, current()->bg); }
uint8_t theme_color_border(void) { return make(current()->accent_fg, current()->bg); }
uint8_t theme_color_selected(void) { return make(current()->selected_fg, current()->selected_bg); }
uint8_t theme_color_dialog(void) { return make(current()->dialog_fg, current()->dialog_bg); }
uint8_t theme_color_dialog_text(void) { return make(current()->dialog_fg, current()->dialog_bg); }
uint8_t theme_color_input(void) { return make(current()->selected_fg, current()->selected_bg); }
uint8_t theme_color_error(void) { return make(current()->error_fg, current()->bg); }
uint8_t theme_color_ok(void) { return make(current()->ok_fg, current()->bg); }
uint8_t theme_color_info(void) { return make(current()->info_fg, current()->bg); }
uint8_t theme_color_panel(void) { return make(current()->fg, current()->bg); }

void theme_set_normal(void) { terminal_set_color(theme_color_normal()); }
void theme_set_title(void) { terminal_set_color(theme_color_title()); }
void theme_set_dir_tag(void) { terminal_set_color(theme_color_footer()); }
void theme_set_dir_name(void) { terminal_set_color(theme_color_normal()); }
void theme_set_file_tag(void) { terminal_set_color(make(current()->muted_fg, current()->bg)); }
void theme_set_root(void) { terminal_set_color(theme_color_title()); }
void theme_set_tree_line(void) { terminal_set_color(theme_color_border()); }
void theme_set_error(void) { terminal_set_color(theme_color_error()); }

void theme_print_banner(void) {
    theme_set_title();
    kprintf("                %s\n", LOS_VERSION);
    theme_set_normal();
}

void theme_log_ok(const char *message) {
    terminal_set_color(theme_color_ok());
    kprintf("[OK] ");
    theme_set_normal();
    kprintf("%s\n", message);
}

void theme_log_info(const char *message) {
    terminal_set_color(theme_color_info());
    kprintf("[..] ");
    theme_set_normal();
    kprintf("%s\n", message);
}

void theme_log_error(const char *message) {
    terminal_set_color(theme_color_error());
    kprintf("[ERR] ");
    theme_set_normal();
    kprintf("%s\n", message);
}

void theme_prompt(void) {
    terminal_set_color(make(current()->accent_fg, current()->bg));
    kprintf("LOS ");
    theme_set_normal();
}

void theme_list(void) {
    theme_ensure_default();

    kprintf("Themes:\n");

    for (int i = 0; i < THEME_COUNT; i++) {
        if (i == active_theme) {
            terminal_set_color(theme_color_ok());
            kprintf("[x] ");
        } else {
            theme_set_normal();
            kprintf("[ ] ");
        }

        kprintf("%s", themes[i].name);
        theme_set_normal();
        kprintf(" - %s\n", themes[i].label);
    }

    theme_set_normal();
}

int theme_set_scheme(const char *name) {
    if (!name || !name[0]) return 0;

    int index = theme_find_index(name);
    if (index < 0) return 0;

    active_theme = index;
    theme_set_normal();
    return 1;
}

const char *theme_current_name(void) {
    return current()->name;
}

void theme_clear_screen(void) {
    uint8_t color = theme_color_normal();

    for (size_t y = 0; y < 25; y++) {
        for (size_t x = 0; x < 80; x++) {
            terminal_putentry_at(' ', color, x, y);
        }
    }

    terminal_move_cursor(0, 0);
    theme_set_normal();
}

void theme_next(void) {
    theme_ensure_default();
    active_theme++;
    if (active_theme >= THEME_COUNT) active_theme = 0;
    theme_set_normal();
}

void theme_prev(void) {
    theme_ensure_default();
    active_theme--;
    if (active_theme < 0) active_theme = THEME_COUNT - 1;
    theme_set_normal();
}

int theme_count(void) {
    return THEME_COUNT;
}

int theme_current_index(void) {
    theme_ensure_default();
    return active_theme;
}

const char *theme_name_at(int index) {
    if (index < 0 || index >= THEME_COUNT) return "";
    return themes[index].name;
}

const char *theme_label_at(int index) {
    if (index < 0 || index >= THEME_COUNT) return "";
    return themes[index].label;
}

int theme_set_index(int index) {
    if (index < 0 || index >= THEME_COUNT) return 0;
    active_theme = index;
    theme_set_normal();
    return 1;
}


void theme_repaint_screen(void) {
    uint16_t screen[80 * 25];
    uint8_t color = theme_color_normal();

    terminal_copy_screen_to(screen);

    for (size_t y = 0; y < 25; y++) {
        for (size_t x = 0; x < 80; x++) {
            uint16_t cell = screen[y * 80 + x];
            char ch = (char)(cell & 0xFF);

            terminal_putentry_at(ch, color, x, y);
        }
    }

    theme_set_normal();
}
