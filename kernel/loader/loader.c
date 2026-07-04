#include "../include/loader.h"
#include "../include/app.h"
#include "../include/kprintf.h"
#include "../include/string.h"
#include "../include/eventlog.h"
#include "../include/vfs.h"

static void loaded_hello_app(void) {
    kprintf("Hello from dynamically loaded LAP app.\n");
}

static void loaded_notes_app(void) {
    kprintf("LOS Notes started.\n");
    kprintf("Notes UI coming soon.\n");
}

static void loaded_model_app(void) {
    kprintf("LOS Model Loader started.\n");
    kprintf("GGUF model runtime coming soon.\n");
}

void loader_initialize(void) {
    eventlog_add("lap loader initialized");
}

void loader_list_formats(void) {
    kprintf("Supported executable formats:\n");
    kprintf("  .lap  LOS Application Package\n");
    kprintf("\nLAP file content examples:\n");
    kprintf("  APP HELLO\n");
    kprintf("  APP NOTES\n");
    kprintf("  APP MODEL\n");
}

int loader_load_lap(const char *name) {
    if (strcmp(name, "hello.lap") == 0) {
        app_register("hello", "loaded LAP hello app", loaded_hello_app);
        eventlog_add("lap loaded: hello");
        return 1;
    }

    if (strcmp(name, "notes.lap") == 0) {
        app_register("notes", "loaded LAP notes app", loaded_notes_app);
        eventlog_add("lap loaded: notes");
        return 1;
    }

    if (strcmp(name, "model.lap") == 0) {
        app_register("model", "loaded LAP model manager", loaded_model_app);
        eventlog_add("lap loaded: model");
        return 1;
    }

    return 0;
}

int loader_load_lap_from_vfs(vfs_node_t *cwd, const char *name) {
    vfs_node_t *file = vfs_find_child(cwd, name);

    if (!file || file->type != VFS_FILE || !file->content) {
        return loader_load_lap(name);
    }

    if (strcmp(file->content, "APP HELLO") == 0) {
        app_register("hello", "VFS LAP hello app", loaded_hello_app);
        eventlog_add("vfs lap loaded: hello");
        return 1;
    }

    if (strcmp(file->content, "APP NOTES") == 0) {
        app_register("notes", "VFS LAP notes app", loaded_notes_app);
        eventlog_add("vfs lap loaded: notes");
        return 1;
    }

    if (strcmp(file->content, "APP MODEL") == 0) {
        app_register("model", "VFS LAP model manager", loaded_model_app);
        eventlog_add("vfs lap loaded: model");
        return 1;
    }

    return 0;
}
