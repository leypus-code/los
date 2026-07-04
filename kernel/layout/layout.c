#include "../include/layout.h"
#include "../include/terminal.h"
#include "../include/keycodes.h"
#include "../include/ui.h"
#include "../include/shell.h"
#include "../include/kprintf.h"
#include "../include/service.h"
#include "../include/string.h"
#include "../include/version.h"
#include "../include/theme.h"

static workspace_t current;
static int active_block = 0;

static uint8_t bg, title, footer;

static void colors(void) {
    bg = theme_color_panel();
    title = theme_color_title();
    footer = theme_color_footer();
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

static void clear(void) {
    colors();

    for (size_t y = 0; y < 25; y++) {
        for (size_t x = 0; x < 80; x++) {
            put(x, y, ' ', bg);
        }
    }
}

static void add_block(int type, int x, int y, int w, int h, const char *title_text, const char *content) {
    if (current.block_count >= 24) return;

    ui_block_t *b = &current.blocks[current.block_count++];
    b->type = type;
    b->x = x;
    b->y = y;
    b->w = w;
    b->h = h;
    b->title = title_text;
    b->content = content;
    b->action = 0;
    b->flags = 0;
}

static void layout_draw(void) {
    clear();

    print_at(2, 0, "LOS Intent Workspace", title);
    print_at(45, 0, current.title ? current.title : "Untitled", title);

    for (int i = 0; i < current.block_count; i++) {
        uiblock_draw(&current.blocks[i], i == active_block);
    }

    for (size_t x = 0; x < 80; x++) {
        put(x, 24, ' ', footer);
    }

    print_at(0, 24, "Tab Switch Block   Enter Activate   Q Exit", footer);
    print_at(69, 24, LOS_VERSION, footer);
}

void layout_initialize(void) {
    current.title = "none";
    current.block_count = 0;
    active_block = 0;
}


void layout_open_blocks(const char *workspace_title, ui_block_t *blocks, int count) {
    current.title = workspace_title ? workspace_title : "Workspace";
    current.block_count = 0;
    active_block = 0;

    if (count > 24) count = 24;

    for (int i = 0; i < count; i++) {
        current.blocks[i].type = blocks[i].type;
        current.blocks[i].x = blocks[i].x;
        current.blocks[i].y = blocks[i].y;
        current.blocks[i].w = blocks[i].w;
        current.blocks[i].h = blocks[i].h;
        current.blocks[i].title = blocks[i].title;
        current.blocks[i].content = blocks[i].content;
        current.blocks[i].action = blocks[i].action;
        current.blocks[i].flags = blocks[i].flags;

        if (current.blocks[i].x < 0) current.blocks[i].x = 1;
        if (current.blocks[i].y < 1) current.blocks[i].y = 2;
        if (current.blocks[i].w < 8) current.blocks[i].w = 8;
        if (current.blocks[i].h < 4) current.blocks[i].h = 4;

        if (current.blocks[i].x + current.blocks[i].w > 80) {
            current.blocks[i].w = 80 - current.blocks[i].x;
        }

        if (current.blocks[i].y + current.blocks[i].h > 24) {
            current.blocks[i].h = 24 - current.blocks[i].y;
        }

        current.block_count++;
    }

    terminal_save_screen();
    ui_enter(UI_LAYOUT);
    layout_draw();
}

void layout_open_debug_workspace(void) {
    current.title = "AVEX Debug Workspace";
    current.block_count = 0;
    active_block = 0;

    add_block(UI_BLOCK_STATUS,   1,  2, 24,  6, " Build Status ", "Build failed\nMissing dependency");
    add_block(UI_BLOCK_CODE,    26,  2, 27, 10, " Code ", "app.ts:148\nimport missing\n\nSuggested patch");
    add_block(UI_BLOCK_AI,      54,  2, 25, 10, " AI Analysis ", "Cause detected\nApply patch?\nRun tests");
    add_block(UI_BLOCK_TERMINAL, 1, 13, 38,  6, " Terminal ", "npm run build\nerror: module not found");
    add_block(UI_BLOCK_LOGS,    40, 13, 20,  6, " Logs ", "api exited\ncontainer running");
    add_block(UI_BLOCK_BUTTON,  61, 13, 18,  6, " Action ", "Apply Fix");

    terminal_save_screen();
    ui_enter(UI_LAYOUT);
    layout_draw();
}

void layout_open_server_workspace(void) {
    current.title = "Server Ops Workspace";
    current.block_count = 0;
    active_block = 0;

    add_block(UI_BLOCK_STATUS,   1,  2, 25,  6, " Server ", "online\nCPU OK\nRAM OK");
    add_block(UI_BLOCK_TERMINAL,27,  2, 26, 10, " SSH ", "ssh srv-admin\nsystemctl status");
    add_block(UI_BLOCK_LOGS,   54,  2, 25, 10, " Logs ", "nginx ok\ndocker ok\nfirewall ok");
    add_block(UI_BLOCK_AI,      1, 13, 38,  6, " AI Ops ", "No critical issues\nSuggested: update packages");
    add_block(UI_BLOCK_BUTTON, 40, 13, 18,  6, " Action ", "Restart");
    add_block(UI_BLOCK_TEXT,   59, 13, 20,  6, " Notes ", "Server workspace\nservice-based UI");

    terminal_save_screen();
    ui_enter(UI_LAYOUT);
    layout_draw();
}

void layout_open_video_workspace(void) {
    current.title = "Video Workspace";
    current.block_count = 0;
    active_block = 0;

    add_block(UI_BLOCK_STATUS,  1,  2, 22,  6, " Project ", "video loaded\nFPS: 60\nDuration: 2:35");
    add_block(UI_BLOCK_TEXT,   24,  2, 30, 10, " Preview ", "Preview service stub\n[ frame output ]");
    add_block(UI_BLOCK_AI,     55,  2, 24, 10, " AI Suggestions ", "Cut silence\nNormalize audio\nExport MP4");
    add_block(UI_BLOCK_CODE,    1, 13, 34,  6, " Timeline ", "00:00 intro\n00:30 scene\n01:20 music");
    add_block(UI_BLOCK_LOGS,   36, 13, 22,  6, " Audio ", "peak -3db\nnoise low");
    add_block(UI_BLOCK_BUTTON, 59, 13, 20,  6, " Export ", "Render");

    terminal_save_screen();
    ui_enter(UI_LAYOUT);
    layout_draw();
}



static int action_starts_with(const char *s, const char *prefix) {
    int i = 0;

    if (!s || !prefix) return 0;

    while (prefix[i]) {
        if (s[i] != prefix[i]) return 0;
        i++;
    }

    return 1;
}

static int execute_block_action(const char *action) {
    if (!action || !action[0]) return 0;

    if (action_starts_with(action, "apps.open ")) {
        return service_call("apps", "open", action + 10);
    }

    if (action_starts_with(action, "workspace.open ")) {
        return service_call("workspace", "open", action + 15);
    }

    if (action_starts_with(action, "model.import ")) {
        return service_call("model", "import", action + 13);
    }

    if (action_starts_with(action, "model.load ")) {
        return service_call("model", "load", action + 11);
    }

    if (action_starts_with(action, "fs.mkdir ")) {
        return service_call("fs", "mkdir", action + 9);
    }

    if (action_starts_with(action, "fs.touch ")) {
        return service_call("fs", "touch", action + 9);
    }

    if (action_starts_with(action, "packages.install ")) {
        return service_call("packages", "install", action + 17);
    }

    return 0;
}

void layout_handle_key(int key) {
    if (key == 'q' || key == KEY_ESCAPE || key == KEY_F10) {
        ui_exit_to_shell();
        return;
    }

    if (key == KEY_TAB || key == KEY_ARROW_RIGHT) {
        if (current.block_count > 0) {
            active_block++;
            if (active_block >= current.block_count) active_block = 0;
            layout_draw();
        }
        return;
    }

    if (key == KEY_ARROW_LEFT) {
        if (current.block_count > 0) {
            active_block--;
            if (active_block < 0) active_block = current.block_count - 1;
            layout_draw();
        }
        return;
    }

    if (key == KEY_ENTER) {
        if (current.block_count > 0) {
            ui_block_t *b = &current.blocks[active_block];

            if (b->type == UI_BLOCK_BUTTON && b->action) {
                if (execute_block_action(b->action)) {
                    b->content = "Action executed";
                } else {
                    b->content = "Action failed";
                }
            } else {
                b->content = "Activated\nBlock event stub";
            }

            layout_draw();
        }
        return;
    }
}

void layout_status(void) {
    kprintf("Workspace: %s\n", current.title ? current.title : "none");
    kprintf("Blocks: %u\n", (uint32_t)current.block_count);
    kprintf("Active block: %u\n", (uint32_t)active_block);
}
