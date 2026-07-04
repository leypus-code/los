#include "../include/model.h"
#include "../include/kprintf.h"
#include "../include/string.h"
#include "../include/eventlog.h"
#include "../include/vfs.h"

static vfs_node_t *models_dir = 0;
static vfs_node_t *ai_dir = 0;
static vfs_node_t *active_model = 0;

static vfs_node_t *ensure_dir(vfs_node_t *parent, const char *name) {
    vfs_node_t *dir = vfs_find_child(parent, name);

    if (!dir) {
        dir = vfs_mkdir(parent, name);
    }

    return dir;
}

void model_initialize(void) {
    vfs_node_t *root = vfs_get_root();

    models_dir = ensure_dir(root, "models");
    ai_dir = ensure_dir(root, "ai");

    ensure_dir(ai_dir, "prompts");
    ensure_dir(ai_dir, "memory");
    ensure_dir(ai_dir, "cache");

    vfs_node_t *config = vfs_find_child(ai_dir, "config.ini");
    if (!config) {
        config = vfs_create_file(ai_dir, "config.ini");
        if (config) {
            vfs_write_file(config, "model=none\nmode=local\n");
        }
    }

    active_model = 0;
    eventlog_add("model filesystem initialized");
}

void model_status(void) {
    kprintf("Model Manager: ready\n");
    kprintf("Storage: /models\n");
    kprintf("AI config: /ai/config.ini\n");
    kprintf("Active model: %s\n", active_model ? active_model->name : "none");
}

void model_list(void) {
    kprintf("Models in /models:\n");

    if (!models_dir || !models_dir->children) {
        kprintf("  none\n");
        return;
    }

    vfs_node_t *child = models_dir->children;
    while (child) {
        if (child->type == VFS_FILE) {
            kprintf("  %s\n", child->name);
        }
        child = child->next;
    }
}

int model_import(const char *name) {
    if (!models_dir) {
        return 0;
    }

    vfs_node_t *file = vfs_find_child(models_dir, name);

    if (!file) {
        file = vfs_create_file(models_dir, name);
    }

    if (!file) {
        return 0;
    }

    vfs_write_file(file, "GGUF-STUB\n");
    eventlog_add("model file imported");
    return 1;
}

int model_load(const char *name) {
    if (!models_dir) {
        return 0;
    }

    vfs_node_t *file = vfs_find_child(models_dir, name);

    if (!file || file->type != VFS_FILE) {
        return 0;
    }

    active_model = file;
    eventlog_add("model file activated");
    return 1;
}
