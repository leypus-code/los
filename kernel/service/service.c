#include "../include/service.h"
#include "../include/kprintf.h"
#include "../include/string.h"
#include "../include/eventlog.h"
#include "../include/app.h"
#include "../include/model.h"
#include "../include/package.h"
#include "../include/vfs.h"
#include "../include/layout.h"
#include "../include/workspace_builder.h"

void service_initialize(void) {
    eventlog_add("service bus initialized");
}

void service_list(void) {
    kprintf("System services:\n");
    kprintf("  apps.open <name>\n");
    kprintf("  apps.list\n");
    kprintf("  model.load <name>\n");
    kprintf("  model.import <name>\n");
    kprintf("  model.list\n");
    kprintf("  model.status\n");
    kprintf("  packages.list\n");
    kprintf("  packages.install <lap>\n");
    kprintf("  packages.remove <name>\n");
    kprintf("  fs.mkdir <name>\n");
    kprintf("  fs.touch <name>\n");
    kprintf("  workspace.open <coding|server|video|file.workspace>\n");
    kprintf("  workspace.list\n");
    kprintf("  workspace.create <name.workspace>\n");
    kprintf("  workspace.template <kind> <file.workspace>\n");
}

int service_call(const char *service, const char *action, const char *arg) {

    if (strcmp(service, "workspace") == 0 && strcmp(action, "open") == 0) {
        if (strcmp(arg, "coding") == 0 || strcmp(arg, "debug") == 0) {
            return workspace_builder_open("coding.workspace");
        }

        if (strcmp(arg, "server") == 0) {
            return workspace_builder_open("server.workspace");
        }

        if (strcmp(arg, "video") == 0) {
            return workspace_builder_open("video.workspace");
        }

        return workspace_builder_open(arg);
    }

    if (strcmp(service, "workspace") == 0 && strcmp(action, "list") == 0) {
        workspace_builder_list();
        return 1;
    }

    if (strcmp(service, "workspace") == 0 && strcmp(action, "create") == 0) {
        return workspace_builder_create(arg, "custom");
    }

    if (strcmp(service, "workspace") == 0 && strcmp(action, "template") == 0) {
        char kind[32];
        char name[96];
        int i = 0;
        int j = 0;

        if (!arg || !arg[0]) {
            return 0;
        }

        while (arg[i] && arg[i] != ' ' && i < 31) {
            kind[i] = arg[i];
            i++;
        }
        kind[i] = '\0';

        while (arg[i] == ' ') {
            i++;
        }

        while (arg[i] && j < 95) {
            name[j++] = arg[i++];
        }
        name[j] = '\0';

        if (!kind[0] || !name[0]) {
            return 0;
        }

        return workspace_builder_template(kind, name);
    }

    if (strcmp(service, "apps") == 0 && strcmp(action, "open") == 0) {
        return app_run(arg);
    }

    if (strcmp(service, "apps") == 0 && strcmp(action, "list") == 0) {
        app_list();
        return 1;
    }

    if (strcmp(service, "model") == 0 && strcmp(action, "load") == 0) {
        return model_load(arg);
    }

    if (strcmp(service, "model") == 0 && strcmp(action, "list") == 0) {
        model_list();
        return 1;
    }

    if (strcmp(service, "model") == 0 && strcmp(action, "status") == 0) {
        model_status();
        return 1;
    }

    if (strcmp(service, "model") == 0 && strcmp(action, "import") == 0) {
        return model_import(arg);
    }

    if (strcmp(service, "packages") == 0 && strcmp(action, "list") == 0) {
        package_list();
        return 1;
    }

    if (strcmp(service, "packages") == 0 && strcmp(action, "install") == 0) {
        return package_install(vfs_get_root(), arg);
    }

    if (strcmp(service, "packages") == 0 && strcmp(action, "remove") == 0) {
        return package_remove(arg);
    }

    if (strcmp(service, "fs") == 0 && strcmp(action, "mkdir") == 0) {
        vfs_node_t *root = vfs_get_root();
        return vfs_mkdir(root, arg) ? 1 : 0;
    }

    if (strcmp(service, "fs") == 0 && strcmp(action, "touch") == 0) {
        vfs_node_t *root = vfs_get_root();
        return vfs_create_file(root, arg) ? 1 : 0;
    }

    return 0;
}
