#include "../include/ai.h"
#include "../include/kprintf.h"
#include "../include/eventlog.h"
#include "../include/intent.h"

static int ai_ready = 0;
static int model_loaded = 0;

void ai_initialize(void) {
    ai_ready = 1;
    model_loaded = 0;
    eventlog_add("ai runtime initialized");
}

void ai_status(void) {
    kprintf("AI Runtime: %s\n", ai_ready ? "ready" : "offline");
    kprintf("Model:      %s\n", model_loaded ? "loaded" : "not loaded");
    kprintf("Mode:       stub\n");
}

void ai_prompt(const char *text) {
    if (!ai_ready) {
        kprintf("AI Runtime offline\n");
        return;
    }

    kprintf("AI> received: %s\n", text);

    if (intent_handle(text)) {
        kprintf("AI> done\n");
    } else {
        kprintf("AI> I do not understand this intent yet.\n");
    }
}

void ai_boot_sequence(void) {
    kprintf("Starting LOS Agent...\n");
    kprintf("AI: building desktop...\n");
    kprintf("AI: loading workspace...\n");
    kprintf("AI: ready.\n");
}
