#include "../include/app.h"
#include "../include/kprintf.h"
#include "../include/string.h"
#include "../include/norton.h"
#include "../include/shell.h"
#include "../include/task.h"
#include "../include/eventlog.h"
#include "../include/editor.h"
#include "../include/terminal.h"
#include "../include/ui.h"

#define APP_MAX 16

static app_t app_registry[APP_MAX];
static unsigned int app_count = 0;

static void calc_app(void) {
    kprintf("Calculator started.\n");
    kprintf("2 + 2 = 4\n");
}

static void editor_app(void) {
    editor_enter();
}

static void ai_app(void) {
    kprintf("LOS AI Console started.\n");
    kprintf("Model manager coming soon.\n");
}

static void nc_app(void) {
    ui_enter(UI_NORTON);
    norton_enter();
}

int app_register(const char *name, const char *description, app_entry_t entry) {
    if (app_count >= APP_MAX) {
        return 0;
    }

    app_registry[app_count].name = name;
    app_registry[app_count].description = description;
    app_registry[app_count].entry = entry;
    app_count++;

    eventlog_add("application registered");
    return 1;
}

void app_initialize(void) {
    app_count = 0;

    app_register("calc", "simple calculator demo", calc_app);
    app_register("editor", "LOS text editor", editor_app);
    app_register("ai", "AI console / model manager", ai_app);
    app_register("nc", "Norton-style file manager", nc_app);

    kprintf("[OK] Application manager initialized\n");
    eventlog_add("application manager initialized");
}

void app_list(void) {
    kprintf("Applications:\n");

    for (unsigned int i = 0; i < app_count; i++) {
        kprintf("  %s - %s\n", app_registry[i].name, app_registry[i].description);
    }
}

int app_run(const char *name) {
    for (unsigned int i = 0; i < app_count; i++) {
        if (strcmp(name, app_registry[i].name) == 0) {
            task_t *task = task_create(app_registry[i].name);

            if (task) {
                task_set_state(task, TASK_RUNNING);
            }

            kprintf("Launching %s...\n", app_registry[i].name);
            eventlog_add("application launched");

            app_registry[i].entry();

            if (shell_is_ui_mode()) {
                return 1;
            }

            if (task) {
                task_set_state(task, TASK_DEAD);
            }

            return 1;
        }
    }

    return 0;
}


int app_run_file(const char *name, vfs_node_t *file) {
    if (strcmp(name, "editor") == 0) {
        editor_open_file(file);
        return 1;
    }

    return 0;
}
