#include "../include/norton.h"
#include "../include/version.h"
#include "../include/theme.h"
#include "../include/terminal.h"
#include "../include/vfs.h"
#include "../include/shell.h"
#include "../include/keycodes.h"
#include "../include/keyboard.h"
#include "../include/editor.h"
#include "../include/app.h"
#include "../include/ui.h"

#define INPUT_MAX 32
#define PANEL_H 22
#define LEFT_X 0
#define RIGHT_X 40
#define PANEL_W 40

static int active_panel = 0;
static int selected = 0;
static int left_scroll = 0;
static int right_scroll = 0;
static int active = 0;
static int force_clean_exit = 0;
static int has_shell_snapshot = 0;
static uint16_t norton_shell_screen[80 * 25];
static int dialog_mode = 0;
static int delete_confirm_mode = 0;
static int rename_mode = 0;
static int modal_mode = 0;
static vfs_node_t *rename_target = 0;
static vfs_node_t *delete_target = 0;
static char input_buffer[INPUT_MAX];
static int input_len = 0;

static vfs_node_t *left_dir = 0;
static vfs_node_t *right_dir = 0;
static vfs_node_t *norton_start_dir = 0;

static uint8_t border, title, text, selected_color, footer, shadow, input_color, header;

static void norton_draw(void);

void norton_set_start_dir(vfs_node_t *dir) {
    norton_start_dir = dir;
}

static void init_colors(void) {
    border = theme_color_border();
    title = theme_color_title();
    header = theme_color_title();
    text = theme_color_normal();
    selected_color = theme_color_selected();
    footer = theme_color_footer();
    shadow = theme_color_footer();
    input_color = theme_color_input();
}


static int *current_scroll_ptr(void) {
    return active_panel == 0 ? &left_scroll : &right_scroll;
}

static int current_scroll(void) {
    return active_panel == 0 ? left_scroll : right_scroll;
}

static void reset_active_scroll(void) {
    if (active_panel == 0) left_scroll = 0;
    else right_scroll = 0;
}

static void ensure_selected_visible(void) {
    int *scroll = current_scroll_ptr();

    if (selected < *scroll) {
        *scroll = selected;
    }

    if (selected >= *scroll + 34) {
        *scroll = selected - 33;
    }

    if (*scroll < 0) {
        *scroll = 0;
    }
}

static vfs_node_t *current_dir(void) {
    return active_panel == 0 ? left_dir : right_dir;
}

static void put(size_t x, size_t y, char c, uint8_t color) {
    terminal_putentry_at(c, color, x, y);
}

static void print_at(size_t x, size_t y, const char *s, uint8_t color) {
    for (size_t i = 0; s[i]; i++) {
        if (x + i < 80) put(x + i, y, s[i], color);
    }
}


static void print_clipped_at(size_t x, size_t y, const char *s, uint8_t color, size_t max) {
    size_t i = 0;
    int truncated = 0;

    if (!s || max == 0) return;

    while (s[i] && i < max) {
        if (s[i + 1] && i == max - 1) {
            truncated = 1;
            break;
        }

        if (x + i < 80) {
            put(x + i, y, s[i], color);
        }

        i++;
    }

    if (truncated && max > 1 && x + max - 1 < 80) {
        put(x + max - 1, y, '~', color);
    }
}

static void fill_rect(size_t x, size_t y, size_t w, size_t h, uint8_t color) {
    for (size_t yy = y; yy < y + h; yy++)
        for (size_t xx = x; xx < x + w; xx++)
            put(xx, yy, ' ', color);
}

static void hline(size_t x, size_t y, size_t w, char c, uint8_t color) {
    for (size_t i = 0; i < w; i++) put(x + i, y, c, color);
}

static void vline(size_t x, size_t y, size_t h, char c, uint8_t color) {
    for (size_t i = 0; i < h; i++) put(x, y + i, c, color);
}


static int str_len(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void make_compact_path(const char *src, char *dst, int max) {
    int len = str_len(src);

    if (len < max) {
        int i = 0;
        while (src[i]) {
            dst[i] = src[i];
            i++;
        }
        dst[i] = '\0';
        return;
    }

    dst[0] = '/';
    dst[1] = '.';
    dst[2] = '.';
    dst[3] = '.';

    int tail = max - 5;
    int start = len - tail;
    int d = 4;

    for (int i = start; src[i] && d < max - 1; i++) {
        dst[d++] = src[i];
    }

    dst[d] = '\0';
}

static void draw_panel(size_t x, const char *drive, vfs_node_t *dir, int is_active) {
    uint8_t c = is_active ? title : border;
    char path[64];
    char compact[28];

    vfs_path(dir, path, 64);
    make_compact_path(path, compact, 24);

    fill_rect(x, 0, PANEL_W, PANEL_H, theme_color_panel());

    put(x, 0, '+', c);
    put(x + PANEL_W - 1, 0, '+', c);
    put(x, PANEL_H - 1, '+', c);
    put(x + PANEL_W - 1, PANEL_H - 1, '+', c);

    hline(x + 1, 0, PANEL_W - 2, '-', c);
    hline(x + 1, PANEL_H - 1, PANEL_W - 2, '-', c);
    vline(x, 1, PANEL_H - 2, '|', c);
    vline(x + PANEL_W - 1, 1, PANEL_H - 2, '|', c);
    vline(x + 19, 1, PANEL_H - 2, '|', border);

    print_at(x + 3, 0, drive, title);
    print_at(x + 10, 0, compact, title);

    print_at(x + 2, 2, "Name", header);
    print_at(x + 22, 2, "Name", header);

    hline(x + 1, 3, PANEL_W - 2, '-', border);
    put(x, 3, '+', border);
    put(x + 19, 3, '+', border);
    put(x + PANEL_W - 1, 3, '+', border);
}


static int child_count(vfs_node_t *dir) {
    int count = dir->parent ? 1 : 0;

    vfs_node_t *c = dir->children;
    while (c) {
        count++;
        c = c->next;
    }

    return count;
}

static vfs_node_t *child_at(vfs_node_t *dir, int index) {
    int offset = dir->parent ? 1 : 0;

    if (offset && index == 0) {
        return dir->parent;
    }

    vfs_node_t *c = dir->children;
    int i = offset;

    while (c) {
        if (i == index) return c;
        c = c->next;
        i++;
    }

    return 0;
}

static vfs_node_t *selected_node(void) {
    return child_at(current_dir(), selected);
}


static void draw_file_cell(size_t x, size_t y, vfs_node_t *node, int index, int is_selected) {
    uint8_t row_color = is_selected ? selected_color : text;

    for (size_t i = 0; i < 17; i++) {
        put(x + i, y, ' ', row_color);
    }

    if (node && index == 0 && node->children != 0 && node == current_dir()->parent) {
        print_at(x, y, "..", row_color);
        return;
    }

    if (!node) {
        return;
    }

    if (!is_selected && node->type == VFS_FILE) {
        row_color = theme_color_normal();
        for (size_t i = 0; i < 17; i++) {
            put(x + i, y, ' ', row_color);
        }
    }

    print_clipped_at(x, y, node->name, row_color, 17);
}

static void draw_panel_files(size_t x, vfs_node_t *dir, int is_active) {
    int scroll = 0;

    if (is_active) {
        scroll = current_scroll();
    } else {
        scroll = (dir == left_dir) ? left_scroll : right_scroll;
    }

    int index = scroll;
    int row = 4;

    while (row < 21) {
        vfs_node_t *left_node = child_at(dir, index);
        vfs_node_t *right_node = child_at(dir, index + 17);

        draw_file_cell(x + 2, row, left_node, index, is_active && selected == index);
        draw_file_cell(x + 21, row, right_node, index + 17, is_active && selected == index + 17);

        row++;
        index++;
    }

    int total = child_count(dir);
    if (total > 34) {
        uint8_t info = theme_color_info();
        print_at(x + 30, 21, "more", info);
    }
}

static void footer_append_char(char *buf, int *pos, int max, char c) {
    if (*pos < max - 1) {
        buf[*pos] = c;
        (*pos)++;
        buf[*pos] = '\0';
    }
}

static void footer_append_text(char *buf, int *pos, int max, const char *text) {
    for (int i = 0; text && text[i] && *pos < max - 1; i++) {
        buf[*pos] = text[i];
        (*pos)++;
    }

    buf[*pos] = '\0';
}

static void footer_append_uint(char *buf, int *pos, int max, int value) {
    char tmp[12];
    int n = 0;

    if (value <= 0) {
        footer_append_char(buf, pos, max, '0');
        return;
    }

    while (value > 0 && n < 11) {
        tmp[n++] = '0' + (value % 10);
        value = value / 10;
    }

    for (int i = n - 1; i >= 0; i--) {
        footer_append_char(buf, pos, max, tmp[i]);
    }
}

static void draw_footer_status(void) {
    char path[64];
    char compact[22];
    char status[40];
    int pos = 0;

    vfs_node_t *dir = current_dir();
    int files = child_count(dir);

    vfs_path(dir, path, 64);
    make_compact_path(path, compact, 20);

    footer_append_text(status, &pos, 40, "F:");
    footer_append_uint(status, &pos, 40, files);
    footer_append_char(status, &pos, 40, ' ');
    footer_append_text(status, &pos, 40, compact);
    footer_append_char(status, &pos, 40, ' ');
    footer_append_text(status, &pos, 40, LOS_VERSION);

    print_clipped_at(50, 24, status, footer, 30);
}

static void draw_footer(void) {
    for (size_t x = 0; x < 80; x++) put(x, 24, ' ', footer);

    print_at(0, 24,  "Enter Open", footer);
    print_at(12, 24, "Bksp Up", footer);
    print_at(21, 24, "Tab", footer);
    print_at(27, 24, "M Mkdir", footer);
    print_at(36, 24, "R Ren", footer);
    print_at(43, 24, "D Del", footer);

    draw_footer_status();
}

static void draw_shadow_rect(size_t x, size_t y, size_t w, size_t h) {
    for (size_t yy = y + 1; yy <= y + h; yy++)
        for (size_t xx = x + 2; xx <= x + w + 1; xx++)
            if (xx < 80 && yy < 25) put(xx, yy, ' ', shadow);
}


static void draw_input_wrapped(size_t x, size_t y, size_t w, size_t h, const char *value) {
    for (size_t yy = 0; yy < h; yy++) {
        for (size_t xx = 0; xx < w; xx++) {
            put(x + xx, y + yy, ' ', input_color);
        }
    }

    size_t cx = 0;
    size_t cy = 0;

    for (size_t i = 0; value && value[i]; i++) {
        if (cy >= h) {
            if (w > 0) put(x + w - 1, y + h - 1, '~', input_color);
            return;
        }

        put(x + cx, y + cy, value[i], input_color);

        cx++;
        if (cx >= w) {
            cx = 0;
            cy++;
        }
    }
}

static void move_input_cursor_wrapped(size_t x, size_t y, size_t w, size_t h, int len) {
    size_t cx = 0;
    size_t cy = 0;

    if (w > 0) {
        cx = len % (int)w;
        cy = len / (int)w;
    }

    if (cy >= h) {
        cy = h - 1;
        cx = w - 1;
    }

    terminal_move_cursor(x + cx, y + cy);
}

static void draw_dialog_box(size_t x, size_t y, size_t w, size_t h, const char *caption) {
    draw_shadow_rect(x, y, w, h);

    put(x, y, '+', border);
    put(x + w - 1, y, '+', border);
    put(x, y + h - 1, '+', border);
    put(x + w - 1, y + h - 1, '+', border);

    hline(x + 1, y, w - 2, '-', border);
    hline(x + 1, y + h - 1, w - 2, '-', border);
    vline(x, y + 1, h - 2, '|', border);
    vline(x + w - 1, y + 1, h - 2, '|', border);

    fill_rect(x + 1, y + 1, w - 2, h - 2, theme_color_dialog());
    print_at(x + 3, y, caption, title);
}


static void print_wrapped_at(size_t x, size_t y, size_t w, size_t h, const char *text, uint8_t color) {
    size_t cx = x;
    size_t cy = y;

    for (size_t i = 0; text && text[i]; i++) {
        if (text[i] == '\n') {
            cx = x;
            cy++;
            if (cy >= y + h) return;
            continue;
        }

        put(cx, cy, text[i], color);
        cx++;

        if (cx >= x + w) {
            cx = x;
            cy++;
            if (cy >= y + h) return;
        }
    }
}


static void draw_file_viewer(vfs_node_t *node) {
    uint8_t dt = theme_color_dialog_text();

    norton_draw();
    draw_dialog_box(10, 4, 60, 16, " File viewer ");

    if (!node) {
        print_at(14, 7, "No file selected", dt);
        modal_mode = 1;
        return;
    }

    print_at(14, 7, "File:", dt);
    print_at(20, 7, node->name, dt);

    for (size_t x = 14; x < 66; x++) {
        put(x, 9, '-', dt);
    }

    if (node->content && node->content[0]) {
        print_wrapped_at(14, 11, 52, 6, node->content, dt);
    } else {
        print_at(14, 11, "(empty file)", dt);
    }

    print_at(32, 18, "[ OK ]", selected_color);
    modal_mode = 1;
}


static void draw_message(const char *caption, const char *message) {
    draw_dialog_box(20, 8, 40, 8, caption);
    print_at(24, 11, message, theme_color_dialog_text());
    print_at(34, 13, "[ OK ]", selected_color);
    modal_mode = 1;
}

static void draw_rename_dialog(void) {
    size_t x = 22, y = 8, w = 36, h = 9;
    uint8_t dialog_text = theme_color_dialog_text();

    draw_dialog_box(x, y, w, h, " Rename ");

    print_at(x + 3, y + 3, "New name:", dialog_text);

    draw_input_wrapped(x + 4, y + 4, 28, 2, input_buffer);

    print_at(x + 9, y + 6, "[ Enter = OK ]", selected_color);
    print_at(x + 9, y + 7, "[ Esc = Cancel ]", dialog_text);

    move_input_cursor_wrapped(x + 4, y + 4, 28, 2, input_len);
}

static void draw_mkdir_dialog(void) {
    size_t x = 22, y = 8, w = 36, h = 9;
    uint8_t dialog_text = theme_color_dialog_text();

    draw_dialog_box(x, y, w, h, " Create directory ");
    print_at(x + 3, y + 3, "Name:", dialog_text);

    draw_input_wrapped(x + 4, y + 4, 28, 2, input_buffer);

    print_at(x + 9, y + 6, "[ Enter = OK ]", selected_color);
    print_at(x + 9, y + 7, "[ Esc = Cancel ]", dialog_text);

    move_input_cursor_wrapped(x + 4, y + 4, 28, 2, input_len);
}

static void norton_draw(void) {
    init_colors();
    fill_rect(0, 0, 80, 25, theme_color_panel());

    draw_panel(LEFT_X, " C:", left_dir, active_panel == 0);
    draw_panel(RIGHT_X, " D:", right_dir, active_panel == 1);

    draw_panel_files(LEFT_X, left_dir, active_panel == 0);
    draw_panel_files(RIGHT_X, right_dir, active_panel == 1);

    draw_footer();
    terminal_move_cursor(0, 23);
}

void norton_enter(void) {
    if (!has_shell_snapshot) {
        terminal_copy_screen_to(norton_shell_screen);
        has_shell_snapshot = 1;
    }

    active = 1;
    active_panel = 0;
    selected = 0;
    left_scroll = 0;
    right_scroll = 0;
    dialog_mode = 0;
    input_len = 0;
    input_buffer[0] = '\0';

    vfs_node_t *start = norton_start_dir ? norton_start_dir : vfs_get_root();

    left_dir = start;
    right_dir = start;
    norton_start_dir = 0;

    norton_draw();
}

static void dialog_handle_key(int c) {
    if (c == KEY_ESCAPE) {
        dialog_mode = 0;
        input_len = 0;
        input_buffer[0] = '\0';
        norton_draw();
        return;
    }

    if (c == KEY_BACKSPACE) {
        if (input_len > 0) {
            input_len--;
            input_buffer[input_len] = '\0';
            norton_draw();
            draw_mkdir_dialog();
        }
        return;
    }

    if (c == KEY_ENTER) {
        if (input_len > 0) {
            vfs_mkdir(current_dir(), input_buffer);
        }

        dialog_mode = 0;
        input_len = 0;
        input_buffer[0] = '\0';
        norton_draw();
        return;
    }

    if (input_len < INPUT_MAX - 1) {
        input_buffer[input_len++] = c;
        input_buffer[input_len] = '\0';
        norton_draw();
        draw_mkdir_dialog();
    }
}

void norton_handle_key(int c) {
    if (!active) return;

    if (modal_mode) {
        if (c == KEY_ENTER || c == KEY_ESCAPE || c == 'q') {
            modal_mode = 0;
            norton_draw();
        }
        return;
    }

    if (rename_mode) {
        if (c == KEY_ESCAPE) {
            rename_mode = 0;
            rename_target = 0;
            input_len = 0;
            input_buffer[0] = '\0';
            norton_draw();
            return;
        }

        if (c == KEY_BACKSPACE) {
            if (input_len > 0) {
                input_len--;
                input_buffer[input_len] = '\0';
                norton_draw();
                draw_rename_dialog();
            }
            return;
        }

        if (c == KEY_ENTER) {
            if (rename_target && input_len > 0) {
                vfs_rename(rename_target, input_buffer);
            }

            rename_mode = 0;
            rename_target = 0;
            input_len = 0;
            input_buffer[0] = '\0';
            norton_draw();
            return;
        }

        if (input_len < INPUT_MAX - 1) {
            input_buffer[input_len++] = c;
            input_buffer[input_len] = '\0';
            norton_draw();
            draw_rename_dialog();
        }

        return;
    }


    if (c == KEY_F1) {
        norton_draw();
        draw_dialog_box(12, 5, 56, 15, " Norton Mode Help ");
        uint8_t dt = theme_color_dialog_text();
        print_at(16, 8,  "Arrow Up/Down   Move selection", dt);
        print_at(16, 9,  "Arrow Left/Right Switch panel", dt);
        print_at(16, 10, "Enter           Open folder/file", dt);
        print_at(16, 11, "Tab             Switch panel", dt);
        print_at(16, 12, "Backspace       Go up", dt);
        print_at(16, 13, "F7 / M          Create directory", dt);
        print_at(16, 14, "F4 / E          Edit selected file", dt);
        print_at(16, 15, "F8 / D          Delete selected", dt);
        print_at(16, 16, "R               Rename selected", dt);
        print_at(16, 17, "F9              System info", dt);
        print_at(16, 18, "F10 / Q / Esc    Exit to shell", dt);
        print_at(33, 20, "[ OK ]", selected_color);
        modal_mode = 1;
        return;
    }

    if (c == KEY_F9) {
        norton_draw();
        draw_dialog_box(18, 7, 44, 10, " System Info ");
        uint8_t dt = theme_color_dialog_text();
        print_at(22, 10, "LOS kernel: v4.9", dt);
        print_at(22, 11, "Mode: Norton UI", dt);
        print_at(22, 12, "FS: RAM VFS", dt);
        print_at(22, 13, "UI: Norton app", dt);
        print_at(22, 14, "Next: Window Manager", dt);
        print_at(34, 15, "[ OK ]", selected_color);
        modal_mode = 1;
        return;
    }

    if (delete_confirm_mode) {
        if (c == 'y') {
            if (delete_target) {
                vfs_delete(delete_target);
            }
            delete_confirm_mode = 0;
            delete_target = 0;
            selected = 0;
            norton_draw();
        } else if (c == 'n' || c == 27 || c == 'q') {
            delete_confirm_mode = 0;
            delete_target = 0;
            norton_draw();
        }
        return;
    }

    if (dialog_mode) {
        dialog_handle_key(c);
        return;
    }

    int count = child_count(current_dir());

    if (c == 'q' || c == KEY_ESCAPE || c == KEY_F10) {
        active = 0;

        terminal_copy_screen_from(norton_shell_screen);
        has_shell_snapshot = 0;
        active = 0;
        shell_set_ui_mode(0);
        theme_repaint_screen();
        terminal_writestring("\n");
        shell_show_prompt();
        return;
    }

    if (c == KEY_TAB || c == KEY_ARROW_LEFT || c == KEY_ARROW_RIGHT) {
        active_panel = active_panel ? 0 : 1;
        selected = 0;
        reset_active_scroll();
        norton_draw();
        return;
    }

    if (c == KEY_BACKSPACE) {
        vfs_node_t *dir = current_dir();
        if (dir && dir->parent) {
            if (active_panel == 0) left_dir = dir->parent;
            else right_dir = dir->parent;
            selected = 0;
            reset_active_scroll();
        }
        norton_draw();
        return;
    }

    if (c == 'w' || c == KEY_ARROW_UP) {
        if (selected > 0) selected--;
        ensure_selected_visible();
        norton_draw();
        return;
    }

    if (c == 's' || c == KEY_ARROW_DOWN) {
        if (selected < count - 1) selected++;
        ensure_selected_visible();
        norton_draw();
        return;
    }

    if (c == 'e' || c == KEY_F4) {
        vfs_node_t *node = selected_node();

        if (!node || node->type != VFS_FILE) {
            draw_message(" Edit ", "Select a file first");
            return;
        }

        app_run_file("editor", node);
        return;
    }

    if (c == 'd' || c == KEY_F8) {
        if (selected == 0 && current_dir()->parent) {
            return;
        }

        vfs_node_t *node = selected_node();

        if (!node) {
            return;
        }

        if (node->children) {
            norton_draw();
            draw_message(" Delete ", "Cannot delete non-empty");
            return;
        }

        delete_target = node;
        delete_confirm_mode = 1;

        norton_draw();
        draw_dialog_box(20, 8, 40, 8, " Delete ");
        print_at(24, 11, "Delete selected item? Y/N", theme_color_dialog_text());
        print_at(31, 13, "[ Y ]   [ N ]", selected_color);
        return;
    }

    if (c == 'm' || c == KEY_F7) {
        dialog_mode = 1;
        input_len = 0;
        input_buffer[0] = '\0';
        norton_draw();
        draw_mkdir_dialog();
        return;
    }

    if (c == 'r') {
        if (selected == 0 && current_dir()->parent) {
            return;
        }

        vfs_node_t *node = selected_node();

        if (!node) {
            return;
        }

        rename_target = node;
        rename_mode = 1;
        input_len = 0;
        input_buffer[0] = '\0';

        norton_draw();
        draw_rename_dialog();
        return;
    }

    if (c == KEY_ENTER) {
        vfs_node_t *node = selected_node();

        if (!node) {
            return;
        }

        if (selected == 0 && current_dir()->parent) {
            vfs_node_t *dir = current_dir();

            if (active_panel == 0) left_dir = dir->parent;
            else right_dir = dir->parent;

            selected = 0;
            reset_active_scroll();
            norton_draw();
            return;
        }

        if (node->type == VFS_DIRECTORY) {
            if (active_panel == 0) left_dir = node;
            else right_dir = node;
            selected = 0;
            reset_active_scroll();
            norton_draw();
        } else if (node->type == VFS_FILE) {
            draw_file_viewer(node);
        }        }

        return;
    }

void norton_resume(void) {
    active = 1;
    ui_enter(UI_NORTON);
    norton_draw();
}

void norton_force_clean_exit(void) {
    force_clean_exit = 0;
}
