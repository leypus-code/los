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

static const char *intent_basename(const char *path) {
    const char *name = path;

    if (!path) {
        return "";
    }

    for (int i = 0; path[i]; i++) {
        if (path[i] == '/') {
            name = path + i + 1;
        }
    }

    return name;
}

static int intent_open_workspace_robust(const char *file) {
    const char *short_name = intent_basename(file);

    kprintf("Intent: workspace.open %s\n", file);

    if (service_call("workspace", "open", file)) {
        return 1;
    }

    if (short_name && short_name[0] && strcmp(short_name, file) != 0) {
        kprintf("Intent: workspace.open retry %s\n", short_name);

        if (service_call("workspace", "open", short_name)) {
            return 1;
        }
    }

    kprintf("Intent: workspace.open failed\n");
    return 0;
}

static int intent_workspace_template(const char *kind, const char *file, int open_after) {
    char spec[128];
    int pos = 0;

    const char *prefix = "";

    for (int i = 0; kind && kind[i] && pos < 126; i++) {
        spec[pos++] = kind[i];
    }

    if (pos < 126) {
        spec[pos++] = ' ';
    }

    for (int i = 0; file && file[i] && pos < 126; i++) {
        spec[pos++] = file[i];
    }

    spec[pos] = '\0';

    kprintf("Intent: workspace.template %s\n", spec);

    if (!service_call("workspace", "template", spec)) {
        kprintf("Intent: workspace.template failed\n");
        return 0;
    }

    kprintf("Intent: workspace.template ok\n");

    if (open_after) {
        if (!intent_open_workspace_robust(file)) {
            return 0;
        }
    }

    return 1;
}

static int intent_workspace_kind(const char *text, const char *kind, const char *file) {
    char create_phrase[64];
    char open_phrase[64];
    char create_open_phrase[96];
    int pos = 0;

    const char *a = "create ";
    const char *b = " workspace";
    const char *c = "open ";
    const char *d = "create and open ";

    pos = 0;
    for (int i = 0; a[i] && pos < 63; i++) create_phrase[pos++] = a[i];
    for (int i = 0; kind[i] && pos < 63; i++) create_phrase[pos++] = kind[i];
    for (int i = 0; b[i] && pos < 63; i++) create_phrase[pos++] = b[i];
    create_phrase[pos] = '\0';

    pos = 0;
    for (int i = 0; c[i] && pos < 63; i++) open_phrase[pos++] = c[i];
    for (int i = 0; kind[i] && pos < 63; i++) open_phrase[pos++] = kind[i];
    for (int i = 0; b[i] && pos < 63; i++) open_phrase[pos++] = b[i];
    open_phrase[pos] = '\0';

    pos = 0;
    for (int i = 0; d[i] && pos < 95; i++) create_open_phrase[pos++] = d[i];
    for (int i = 0; kind[i] && pos < 95; i++) create_open_phrase[pos++] = kind[i];
    for (int i = 0; b[i] && pos < 95; i++) create_open_phrase[pos++] = b[i];
    create_open_phrase[pos] = '\0';

    if (strcmp(text, create_phrase) == 0) {
        if (intent_workspace_template(kind, file, 0)) {
            kprintf("Intent: created %s workspace\n", kind);
        } else {
            kprintf("Intent: create failed for %s workspace\n", kind);
        }

        return 1;
    }

    if (strcmp(text, open_phrase) == 0) {
        if (!intent_open_workspace_robust(file)) {
            kprintf("Intent: workspace open failed, creating template first\n");

            if (!intent_workspace_template(kind, file, 0)) {
                kprintf("Intent: workspace create failed\n");
                return 1;
            }

            intent_open_workspace_robust(file);
        }

        return 1;
    }

    if (strcmp(text, create_open_phrase) == 0) {
        intent_workspace_template(kind, file, 1);
        return 1;
    }

    return 0;
}

int intent_handle(const char *text) {
    if (!text || !text[0]) {
        kprintf("Intent: empty\n");
        return 0;
    }

    if (strcmp(text, "debug build error") == 0) {
        return intent_workspace_template("debug", "/workspaces/debug-build.workspace", 1);
    }

    if (strcmp(text, "debug build") == 0) {
        return intent_workspace_template("debug", "/workspaces/debug-build.workspace", 1);
    }

    if (strcmp(text, "fix build error") == 0) {
        return intent_workspace_template("debug", "/workspaces/debug-build.workspace", 1);
    }

    if (strcmp(text, "system overview") == 0) {
        return intent_workspace_template("overview", "/workspaces/system-overview.workspace", 1);
    }

    if (strcmp(text, "overview") == 0) {
        return intent_workspace_template("overview", "/workspaces/system-overview.workspace", 1);
    }

    if (strcmp(text, "write notes") == 0) {
        return intent_workspace_template("writing", "/workspaces/writing.workspace", 1);
    }

    if (strcmp(text, "notes workspace") == 0) {
        return intent_workspace_template("writing", "/workspaces/writing.workspace", 1);
    }

    if (strcmp(text, "plan project") == 0) {
        return intent_workspace_template("planning", "/workspaces/project-plan.workspace", 1);
    }

    if (strcmp(text, "project plan") == 0) {
        return intent_workspace_template("planning", "/workspaces/project-plan.workspace", 1);
    }

    if (strcmp(text, "service overview") == 0) {
        return intent_workspace_template("services", "/workspaces/services.workspace", 1);
    }

    if (intent_workspace_kind(text, "coding", "/workspaces/coding.workspace")) return 1;
    if (intent_workspace_kind(text, "system", "/workspaces/system.workspace")) return 1;
    if (intent_workspace_kind(text, "notes", "/workspaces/notes.workspace")) return 1;
    if (intent_workspace_kind(text, "services", "/workspaces/services.workspace")) return 1;

    if (strcmp(text, "create workspace") == 0) {
        return intent_workspace_template("coding", "/workspaces/coding.workspace", 0);
    }

    if (strcmp(text, "open workspace") == 0) {
        return intent_open_workspace_robust("/workspaces/coding.workspace");
    }

    if (strcmp(text, "create and open workspace") == 0) {
        return intent_workspace_template("coding", "/workspaces/coding.workspace", 1);
    }

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
