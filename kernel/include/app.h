#ifndef LOS_APP_H
#define LOS_APP_H

#include "vfs.h"

typedef void (*app_entry_t)(void);

typedef struct app {
    const char *name;
    const char *description;
    app_entry_t entry;
} app_t;

void app_initialize(void);
void app_list(void);
int app_run(const char *name);
int app_run_file(const char *name, vfs_node_t *file);
int app_register(const char *name, const char *description, app_entry_t entry);

#endif
