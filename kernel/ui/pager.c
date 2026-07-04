#include "../include/pager.h"
#include "../include/terminal.h"
#include "../include/kprintf.h"
#include "../include/keycodes.h"
#include "../include/theme.h"

#define PAGER_PAGE_LINES 18

static const char *pager_title = 0;
static const char **pager_lines = 0;
static int pager_count = 0;
static int pager_page = 0;
static int pager_is_active = 0;

static void pager_draw(void) {
    theme_clear_screen();

    int max_page = (pager_count + PAGER_PAGE_LINES - 1) / PAGER_PAGE_LINES;
    if (max_page <= 0) max_page = 1;

    theme_set_title();
    kprintf("%s     page %u/%u\n\n", pager_title, (uint32_t)(pager_page + 1), (uint32_t)max_page);
    theme_set_normal();

    int start = pager_page * PAGER_PAGE_LINES;

    for (int i = 0; i < PAGER_PAGE_LINES; i++) {
        int idx = start + i;
        if (idx >= pager_count) break;
        kprintf("%s\n", pager_lines[idx]);
    }

    kprintf("\n[w/s] scroll   [q/Enter/Esc] close");
}

void pager_open(const char *title, const char **lines, int count) {
    terminal_save_screen();

    pager_title = title;
    pager_lines = lines;
    pager_count = count;
    pager_page = 0;
    pager_is_active = 1;
    pager_draw();
}

int pager_active(void) {
    return pager_is_active;
}

void pager_key(int key) {
    int max_page = (pager_count + PAGER_PAGE_LINES - 1) / PAGER_PAGE_LINES - 1;

    if (key == 'q' || key == KEY_ESCAPE || key == KEY_ENTER) {
        pager_is_active = 0;
        terminal_restore_screen();
        return;
    }

    if (key == 'w' || key == KEY_ARROW_UP) {
        if (pager_page > 0) pager_page--;
        pager_draw();
        return;
    }

    if (key == 's' || key == KEY_ARROW_DOWN) {
        if (pager_page < max_page) pager_page++;
        pager_draw();
        return;
    }
}
