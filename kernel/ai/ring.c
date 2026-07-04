#include "../include/ring.h"
#include "../include/kprintf.h"
#include "../include/string.h"
#include "../include/eventlog.h"
#include "../include/intent.h"

#define RING_IDLE      0
#define RING_CHAT      1
#define RING_LISTENING 2
#define RING_THINKING  3
#define RING_DOCKED    4

static int ring_state = RING_IDLE;

static const char *ring_state_name(void) {
    if (ring_state == RING_IDLE) return "idle";
    if (ring_state == RING_CHAT) return "chat";
    if (ring_state == RING_LISTENING) return "listening";
    if (ring_state == RING_THINKING) return "thinking";
    if (ring_state == RING_DOCKED) return "docked";
    return "unknown";
}

void ring_initialize(void) {
    ring_state = RING_IDLE;
    eventlog_add("ai ring initialized");
}

void ring_set_state(const char *state) {
    if (!state) return;

    if (strcmp(state, "idle") == 0) {
        ring_state = RING_IDLE;
        eventlog_add("ring state idle");
        return;
    }

    if (strcmp(state, "chat") == 0 || strcmp(state, "open") == 0) {
        ring_state = RING_CHAT;
        eventlog_add("ring state chat");
        return;
    }

    if (strcmp(state, "listen") == 0 || strcmp(state, "listening") == 0) {
        ring_state = RING_LISTENING;
        eventlog_add("ring state listening");
        return;
    }

    if (strcmp(state, "think") == 0 || strcmp(state, "thinking") == 0) {
        ring_state = RING_THINKING;
        eventlog_add("ring state thinking");
        return;
    }

    if (strcmp(state, "dock") == 0 || strcmp(state, "docked") == 0) {
        ring_state = RING_DOCKED;
        eventlog_add("ring state docked");
        return;
    }
}

void ring_status(void) {
    kprintf("AI Ring\n");
    kprintf("  state: %s\n", ring_state_name());

    if (ring_state == RING_IDLE) {
        kprintf("  visual: centered input bar\n");
        kprintf("  hint: chat \"what you want\"\n");
        return;
    }

    if (ring_state == RING_CHAT) {
        kprintf("  visual: full-screen conversation\n");
        kprintf("  hint: user is typing or speaking\n");
        return;
    }

    if (ring_state == RING_LISTENING) {
        kprintf("  visual: ring pulsing / listening\n");
        kprintf("  hint: voice capture active\n");
        return;
    }

    if (ring_state == RING_THINKING) {
        kprintf("  visual: ring vibrating / AI thinking\n");
        kprintf("  hint: workspace mutation pending\n");
        return;
    }

    if (ring_state == RING_DOCKED) {
        kprintf("  visual: ring docked at screen edge\n");
        kprintf("  hint: workspace ready, ring can reopen chat\n");
        return;
    }
}

int ring_handle_command(const char *command) {
    if (!command || !command[0]) {
        ring_status();
        return 1;
    }

    if (strcmp(command, "status") == 0) {
        ring_status();
        return 1;
    }

    if (strcmp(command, "idle") == 0 ||
        strcmp(command, "chat") == 0 ||
        strcmp(command, "open") == 0 ||
        strcmp(command, "listen") == 0 ||
        strcmp(command, "listening") == 0 ||
        strcmp(command, "think") == 0 ||
        strcmp(command, "thinking") == 0 ||
        strcmp(command, "dock") == 0 ||
        strcmp(command, "docked") == 0) {
        ring_set_state(command);
        ring_status();
        return 1;
    }

    kprintf("Ring: unknown command\n");
    return 0;
}

int ring_chat(const char *text) {
    if (!text || !text[0]) {
        kprintf("Chat: empty\n");
        return 0;
    }

    ring_set_state("chat");
    kprintf("Ring: chat expanded\n");

    ring_set_state("thinking");
    kprintf("AI: %s\n", text);
    kprintf("Ring: thinking\n");

    if (!intent_handle(text)) {
        kprintf("AI: intent failed\n");
        ring_set_state("chat");
        return 0;
    }

    ring_set_state("docked");
    kprintf("Ring: docked\n");
    return 1;
}
