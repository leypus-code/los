#include "../include/fileassoc.h"
#include "../include/string.h"
#include "../include/kprintf.h"
#include "../include/eventlog.h"

#define MAX_ASSOC 32

typedef struct {
    char ext[16];
    file_handler_t handler;
} assoc_t;

static assoc_t assoc[MAX_ASSOC];
static int count = 0;

void fileassoc_initialize(void) {
    count = 0;
    eventlog_add("file associations initialized");
}

int fileassoc_register(const char *ext, file_handler_t handler) {
    if (count >= MAX_ASSOC) {
        return 0;
    }

    int i = 0;
    while (ext[i] && i < 15) {
        assoc[count].ext[i] = ext[i];
        i++;
    }
    assoc[count].ext[i] = '\0';

    assoc[count].handler = handler;
    count++;

    return 1;
}

static const char *extension(const char *name) {
    const char *dot = 0;

    while (*name) {
        if (*name == '.') {
            dot = name;
        }
        name++;
    }

    if (dot) {
        return dot + 1;
    }

    return "";
}

int fileassoc_open(const char *filename) {
    const char *ext = extension(filename);

    for (int i = 0; i < count; i++) {
        if (strcmp(ext, assoc[i].ext) == 0) {
            return assoc[i].handler(filename);
        }
    }

    kprintf("No handler for .%s\n", ext);
    return 0;
}

void fileassoc_list(void) {
    kprintf("Registered handlers:\n");

    for (int i = 0; i < count; i++) {
        kprintf("  .%s\n", assoc[i].ext);
    }
}
