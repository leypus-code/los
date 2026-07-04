#include "../include/package.h"
#include "../include/loader.h"
#include "../include/kprintf.h"
#include "../include/string.h"
#include "../include/eventlog.h"

#define PACKAGE_MAX 16

static const char *packages[PACKAGE_MAX];
static int package_count = 0;

void package_initialize(void) {
    package_count = 0;
    eventlog_add("package manager initialized");
}

static int package_exists(const char *name) {
    for (int i = 0; i < package_count; i++) {
        if (strcmp(packages[i], name) == 0) {
            return 1;
        }
    }

    return 0;
}

int package_install(vfs_node_t *cwd, const char *lap_name) {
    if (package_count >= PACKAGE_MAX) {
        return 0;
    }

    if (!loader_load_lap_from_vfs(cwd, lap_name)) {
        return 0;
    }

    if (!package_exists(lap_name)) {
        packages[package_count++] = lap_name;
    }

    eventlog_add("package installed");
    return 1;
}

int package_remove(const char *app_name) {
    for (int i = 0; i < package_count; i++) {
        if (strcmp(packages[i], app_name) == 0) {
            for (int j = i + 1; j < package_count; j++) {
                packages[j - 1] = packages[j];
            }

            package_count--;
            eventlog_add("package removed");
            return 1;
        }
    }

    return 0;
}

void package_list(void) {
    kprintf("Installed packages:\n");

    if (package_count == 0) {
        kprintf("  none\n");
        return;
    }

    for (int i = 0; i < package_count; i++) {
        kprintf("  %s\n", packages[i]);
    }
}
