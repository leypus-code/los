#include "../include/editor.h"
#include "../include/terminal.h"
#include "../include/keycodes.h"
#include "../include/shell.h"
#include "../include/ui.h"
#include "../include/norton.h"
#include "../include/vfs.h"
#include "../include/theme.h"
#include "../include/kprintf.h"

#define EDITOR_MAX 4096
#define EDITOR_LEFT 2
#define EDITOR_TOP 3
#define EDITOR_RIGHT 78
#define EDITOR_BOTTOM 22

static char buffer[EDITOR_MAX];
static int len = 0;
static int cursor = 0;
static int scroll_line = 0;
static int save_modal = 0;
static vfs_node_t *current_file = 0;

static uint8_t bg;
static uint8_t title;
static uint8_t text;
static uint8_t footer;

static void colors(void) {
    bg = theme_color_panel();
    title = theme_color_title();
    text = theme_color_normal();
    footer = theme_color_footer();
}

static void put(size_t x, size_t y, char c, uint8_t color) {
    terminal_putentry_at(c, color, x, y);
}

static void print_at(size_t x, size_t y, const char *s, uint8_t color) {
    for (size_t i = 0; s[i] && x + i < 80; i++) {
        put(x + i, y, s[i], color);
    }
}

static void editor_clear_screen(void) {
    colors();

    for (size_t y = 0; y < 25; y++) {
        for (size_t x = 0; x < 80; x++) {
            terminal_putentry_at(' ', bg, x, y);
        }
    }

    terminal_move_cursor(EDITOR_LEFT, EDITOR_TOP);
}


static int line_start_for_pos(int pos);

static int editor_visible_lines(void) {
    return EDITOR_BOTTOM - EDITOR_TOP + 1;
}

static int line_for_pos(int pos) {
    int line = 0;

    if (pos < 0) pos = 0;
    if (pos > len) pos = len;

    for (int i = 0; i < pos && i < len; i++) {
        if (buffer[i] == '\n') {
            line++;
        }
    }

    return line;
}

static int pos_for_line_start(int target_line) {
    int line = 0;

    if (target_line <= 0) {
        return 0;
    }

    for (int i = 0; i < len; i++) {
        if (buffer[i] == '\n') {
            line++;

            if (line == target_line) {
                return i + 1;
            }
        }
    }

    return len;
}

static int column_for_pos(int pos) {
    int start = line_start_for_pos(pos);
    return pos - start;
}

static void ensure_cursor_visible(void) {
    int line = line_for_pos(cursor);
    int visible = editor_visible_lines();

    if (scroll_line < 0) {
        scroll_line = 0;
    }

    if (line < scroll_line) {
        scroll_line = line;
    }

    if (line >= scroll_line + visible) {
        scroll_line = line - visible + 1;
    }

    if (scroll_line < 0) {
        scroll_line = 0;
    }
}

static void buffer_pos_to_xy(int pos, int *out_x, int *out_y) {
    int line = line_for_pos(pos);
    int start = line_start_for_pos(pos);
    int col = pos - start;

    int x = EDITOR_LEFT + col;
    int y = EDITOR_TOP + (line - scroll_line);

    if (x >= EDITOR_RIGHT) {
        x = EDITOR_RIGHT - 1;
    }

    if (y < EDITOR_TOP) {
        y = EDITOR_TOP;
    }

    if (y > EDITOR_BOTTOM) {
        y = EDITOR_BOTTOM;
    }

    *out_x = x;
    *out_y = y;
}

static int line_start_for_pos(int pos) {
    int i = pos;

    while (i > 0 && buffer[i - 1] != '\n') {
        i--;
    }

    return i;
}

static int line_end_for_pos(int pos) {
    int i = pos;

    while (i < len && buffer[i] != '\n') {
        i++;
    }

    return i;
}

static int visual_column_for_pos(int pos) {
    return pos - line_start_for_pos(pos);
}

static int pos_from_line_column(int line_start, int column) {
    int i = line_start;
    int c = 0;

    while (i < len && buffer[i] != '\n' && c < column) {
        i++;
        c++;
    }

    return i;
}

static int previous_line_start(int pos) {
    int current_start = line_start_for_pos(pos);

    if (current_start == 0) {
        return 0;
    }

    int i = current_start - 1;

    while (i > 0 && buffer[i - 1] != '\n') {
        i--;
    }

    return i;
}

static int next_line_start(int pos) {
    int end = line_end_for_pos(pos);

    if (end >= len) {
        return line_start_for_pos(pos);
    }

    return end + 1;
}

static void draw(void) {
    colors();
    ensure_cursor_visible();
    editor_clear_screen();

    print_at(2, 0, "LOS Editor", title);
    print_at(55, 0, "file:", title);
    if (current_file) {
        print_at(61, 0, current_file->name, title);
    } else {
        print_at(61, 0, "editor.txt", title);
    }

    for (size_t x = 0; x < 80; x++) {
        put(x, 1, '-', title);
        put(x, 23, '-', title);
        put(x, 24, ' ', footer);
    }

    int start_pos = pos_for_line_start(scroll_line);
    int x = EDITOR_LEFT;
    int y = EDITOR_TOP;

    for (int i = start_pos; i < len; i++) {
        if (buffer[i] == '\n') {
            x = EDITOR_LEFT;
            y++;
        } else {
            put(x, y, buffer[i], text);
            x++;

            if (x >= EDITOR_RIGHT) {
                x = EDITOR_LEFT;
                y++;
            }
        }

        if (y > EDITOR_BOTTOM) {
            break;
        }
    }

    int line = line_for_pos(cursor);
    int col = column_for_pos(cursor);

    terminal_set_color(footer);
    terminal_move_cursor(0, 24);
    kprintf("F2 Save   F10 Exit   PgUp/PgDn Scroll   Ln %u Col %u   BUF 4096",
        (uint32_t)(line + 1),
        (uint32_t)(col + 1)
    );

    int cx = EDITOR_LEFT;
    int cy = EDITOR_TOP;
    buffer_pos_to_xy(cursor, &cx, &cy);

    terminal_move_cursor(cx, cy);
}

static void draw_save_modal(void) {
    uint8_t box = theme_color_dialog();
    uint8_t sel = theme_color_selected();

    for (size_t y = 9; y < 15; y++) {
        for (size_t x = 25; x < 55; x++) {
            put(x, y, ' ', box);
        }
    }

    print_at(33, 10, "File saved", box);
    print_at(34, 12, "[ OK ]", sel);
}

static void save_file(void) {
    vfs_node_t *file = current_file;

    if (!file) {
        vfs_node_t *root = vfs_get_root();
        file = vfs_find_child(root, "editor.txt");

        if (!file) {
            file = vfs_create_file(root, "editor.txt");
        }

        current_file = file;
    }

    if (file) {
        vfs_write_file(file, buffer);
    }

    save_modal = 1;
    draw();
    draw_save_modal();
}

static void insert_char(char c) {
    if (len >= EDITOR_MAX - 1) {
        return;
    }

    for (int i = len; i > cursor; i--) {
        buffer[i] = buffer[i - 1];
    }

    buffer[cursor] = c;
    len++;
    cursor++;
    buffer[len] = '\0';
}

static void backspace_char(void) {
    if (cursor <= 0 || len <= 0) {
        return;
    }

    for (int i = cursor - 1; i < len; i++) {
        buffer[i] = buffer[i + 1];
    }

    cursor--;
    len--;
    buffer[len] = '\0';
}

static void editor_exit(void) {
    int previous = ui_pop();

    if (previous == UI_NORTON) {
        norton_resume();
        return;
    }

    shell_resume_from_ui();
}

void editor_enter(void) {
    current_file = 0;
    terminal_save_screen();

    len = 0;
    cursor = 0;
    scroll_line = 0;
    buffer[0] = '\0';

    ui_enter(UI_EDITOR);

    editor_clear_screen();
    draw();
}

void editor_open_file(vfs_node_t *file) {
    current_file = file;
    terminal_save_screen();

    len = 0;
    cursor = 0;
    scroll_line = 0;
    buffer[0] = '\0';

    if (file && file->content) {
        int i = 0;

        /*
         * Leave one byte for NUL and one spare byte so existing files
         * near the limit still remain editable.
         */
        while (file->content[i] && len < EDITOR_MAX - 2) {
            buffer[len++] = file->content[i++];
        }

        buffer[len] = '\0';

        /*
         * Existing files open at the beginning.
         * This is closer to normal nano/editor behavior.
         */
        cursor = 0;
    }

    ui_push(UI_EDITOR);

    editor_clear_screen();
    draw();
}

void editor_handle_key(int key) {
    if (save_modal) {
        if (key == KEY_ENTER || key == KEY_ESCAPE || key == KEY_F10) {
            save_modal = 0;

            if (key == KEY_F10 || key == KEY_ESCAPE) {
                editor_exit();
                return;
            }

            draw();
        }

        return;
    }

    if (key == KEY_F10 || key == KEY_ESCAPE) {
        editor_exit();
        return;
    }

    if (key == KEY_F2) {
        save_file();
        return;
    }

    if (key == KEY_ARROW_LEFT) {
        if (cursor > 0) cursor--;
        draw();
        return;
    }

    if (key == KEY_ARROW_RIGHT) {
        if (cursor < len) cursor++;
        draw();
        return;
    }

    if (key == KEY_ARROW_UP) {
        int col = visual_column_for_pos(cursor);
        int prev_start = previous_line_start(cursor);
        cursor = pos_from_line_column(prev_start, col);
        draw();
        return;
    }

    if (key == KEY_ARROW_DOWN) {
        int col = visual_column_for_pos(cursor);
        int next_start = next_line_start(cursor);
        cursor = pos_from_line_column(next_start, col);
        draw();
        return;
    }

    if (key == KEY_PAGE_UP) {
        int visible = editor_visible_lines();

        scroll_line -= visible;
        if (scroll_line < 0) {
            scroll_line = 0;
        }

        cursor = pos_for_line_start(scroll_line);
        draw();
        return;
    }

    if (key == KEY_PAGE_DOWN) {
        int visible = editor_visible_lines();

        scroll_line += visible;
        cursor = pos_for_line_start(scroll_line);
        ensure_cursor_visible();
        draw();
        return;
    }

    if (key == KEY_BACKSPACE) {
        backspace_char();
        draw();
        return;
    }

    if (key == KEY_ENTER) {
        insert_char('\n');
        draw();
        return;
    }

    if (key >= 32 && key < 127) {
        insert_char((char)key);
        draw();
    }
}
