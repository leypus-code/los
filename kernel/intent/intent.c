#include "../include/intent.h"
#include "../include/service.h"
#include "../include/kprintf.h"
#include "../include/string.h"
#include "../include/eventlog.h"
#include "../include/layout.h"

static int starts_with(const char *s, const char *prefix) {
    int i = 0;

    while (prefix[i]) {
        if (s[i] != prefix[i]) {
            return 0;
        }

        i++;
    }

    return 1;
}


static int ends_with(const char *s, const char *suffix) {
    int slen = strlen(s);
    int suffix_len = strlen(suffix);

    if (slen < suffix_len) return 0;

    return strcmp(s + slen - suffix_len, suffix) == 0;
}


void intent_initialize(void) {
    eventlog_add("intent engine initialized");
}

int intent_handle(const char *text) {

    if (!starts_with(text, "open ") && ends_with(text, ".workspace")) {
        kprintf("Intent: workspace.open %s\n", text);
        return service_call("workspace", "open", text);
    }

    if (starts_with(text, "workspace coding") || starts_with(text, "workspace debug")) {
        kprintf("Intent: workspace.debug\n");
        return service_call("workspace", "open", "coding");
    }

    if (starts_with(text, "workspace server")) {
        kprintf("Intent: workspace.server\n");
        return service_call("workspace", "open", "server");
    }

    if (starts_with(text, "workspace video")) {
        kprintf("Intent: workspace.video\n");
        return service_call("workspace", "open", "video");
    }

    if (starts_with(text, "open ")) {
        const char *target = text + 5;

        if (ends_with(target, ".workspace")) {
            kprintf("Intent: workspace.open %s\n", target);
            return service_call("workspace", "open", target);
        }

        kprintf("Intent: apps.open %s\n", target);
        return service_call("apps", "open", target);
    }

    if (starts_with(text, "create folder ")) {
        const char *name = text + 14;
        kprintf("Intent: fs.mkdir %s\n", name);
        return service_call("fs", "mkdir", name);
    }

    if (starts_with(text, "create file ")) {
        const char *name = text + 12;
        kprintf("Intent: fs.touch %s\n", name);
        return service_call("fs", "touch", name);
    }

    if (starts_with(text, "import model ")) {
        const char *name = text + 13;
        kprintf("Intent: model.import %s\n", name);
        return service_call("model", "import", name);
    }

    if (starts_with(text, "load model ")) {
        const char *name = text + 11;
        kprintf("Intent: model.load %s\n", name);
        return service_call("model", "load", name);
    }

    if (starts_with(text, "show models")) {
        kprintf("Intent: model.list\n");
        return service_call("model", "list", "");
    }

    if (starts_with(text, "show apps")) {
        kprintf("Intent: apps.list\n");
        return service_call("apps", "list", "");
    }

    if (starts_with(text, "show services")) {
        kprintf("Intent: services.list\n");
        service_list();
        return 1;
    }

    kprintf("Intent: unknown\n");
    return 0;
}
