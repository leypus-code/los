#include "../include/uiblock.h"
#include "../include/theme.h"
#include "../include/terminal.h"

static uint8_t border, active_border, title_color, text_color, button_color;

static void colors(void) {
    border = theme_color_border();
    active_border = theme_color_title();
    title_color = theme_color_title();
    text_color = theme_color_normal();
    button_color = theme_color_selected();
}



static void put(size_t x, size_t y, char c, uint8_t color) {
    if (x < 80 && y < 25) {
        terminal_putentry_at(c, color, x, y);
    }
}

static void print_at(size_t x, size_t y, const char *s, uint8_t color) {
    for (size_t i = 0; s && s[i] && x + i < 80; i++) {
        put(x + i, y, s[i], color);
    }
}

static const char *type_name(int type) {
    if (type == UI_BLOCK_TEXT) return "Text";
    if (type == UI_BLOCK_STATUS) return "Status";
    if (type == UI_BLOCK_BUTTON) return "Button";
    if (type == UI_BLOCK_TERMINAL) return "Terminal";
    if (type == UI_BLOCK_CODE) return "Code";
    if (type == UI_BLOCK_LOGS) return "Logs";
    if (type == UI_BLOCK_LIST) return "List";
    if (type == UI_BLOCK_AI) return "AI";
    return "Block";
}

static void draw_frame(ui_block_t *b, int active) {
    uint8_t c = active ? active_border : border;
    int x = b->x;
    int y = b->y;
    int w = b->w;
    int h = b->h;

    put(x, y, '+', c);
    put(x + w - 1, y, '+', c);
    put(x, y + h - 1, '+', c);
    put(x + w - 1, y + h - 1, '+', c);

    for (int i = 1; i < w - 1; i++) {
        put(x + i, y, '-', c);
        put(x + i, y + h - 1, '-', c);
    }

    for (int i = 1; i < h - 1; i++) {
        put(x, y + i, '|', c);
        put(x + w - 1, y + i, '|', c);
    }

    print_at(x + 2, y, b->title ? b->title : type_name(b->type), title_color);
}

static void print_wrapped(int x, int y, int w, int h, const char *s, uint8_t color) {
    int cx = x;
    int cy = y;

    for (int i = 0; s && s[i]; i++) {
        if (s[i] == '\n') {
            cx = x;
            cy++;
            if (cy >= y + h) return;
            continue;
        }

        put(cx, cy, s[i], color);
        cx++;

        if (cx >= x + w) {
            cx = x;
            cy++;
            if (cy >= y + h) return;
        }
    }
}

void uiblock_draw(ui_block_t *b, int active) {
    colors();
    draw_frame(b, active);

    int ix = b->x + 2;
    int iy = b->y + 2;
    int iw = b->w - 4;
    int ih = b->h - 4;

    if (b->type == UI_BLOCK_STATUS) {
        print_at(ix, iy, "[status]", title_color);
        print_wrapped(ix, iy + 2, iw, ih - 2, b->content, text_color);
        return;
    }

    if (b->type == UI_BLOCK_BUTTON) {
        print_at(ix, iy, "[ ", button_color);
        print_at(ix + 2, iy, b->content ? b->content : "OK", button_color);
        print_at(ix + 2 + 12, iy, " ]", button_color);

        if (b->action && b->action[0]) {
            print_at(ix, iy + 2, "action:", title_color);
            print_wrapped(ix, iy + 3, iw, ih - 3, b->action, text_color);
        }

        return;
    }

    if (b->type == UI_BLOCK_CODE) {
        print_at(ix, iy, "code:", title_color);
        print_wrapped(ix, iy + 2, iw, ih - 2, b->content, text_color);
        return;
    }

    if (b->type == UI_BLOCK_TERMINAL) {
        print_at(ix, iy, "$ terminal", title_color);
        print_wrapped(ix, iy + 2, iw, ih - 2, b->content, text_color);
        return;
    }

    if (b->type == UI_BLOCK_LOGS) {
        print_at(ix, iy, "logs:", title_color);
        print_wrapped(ix, iy + 2, iw, ih - 2, b->content, text_color);
        return;
    }

    if (b->type == UI_BLOCK_AI) {
        print_at(ix, iy, "AI:", title_color);
        print_wrapped(ix, iy + 2, iw, ih - 2, b->content, text_color);
        return;
    }

    print_wrapped(ix, iy, iw, ih, b->content ? b->content : type_name(b->type), text_color);
}
