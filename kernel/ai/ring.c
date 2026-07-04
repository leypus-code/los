#include "../include/ring.h"
#include "../include/kprintf.h"
#include "../include/string.h"
#include "../include/eventlog.h"
#include "../include/intent.h"
#include "../include/service.h"
#include "../include/workspace_builder.h"

#define RING_IDLE      0
#define RING_CHAT      1
#define RING_LISTENING 2
#define RING_THINKING  3
#define RING_DOCKED    4

static int ring_state = RING_IDLE;

#define RING_OP_COUNT 8
#define RING_OP_LEN   96

static char ring_ops[RING_OP_COUNT][RING_OP_LEN];
static int ring_op_count = 0;

static void ring_copy_text(char *dst, const char *src, int max) {
    int i = 0;

    if (!dst || max <= 0) return;

    if (!src) {
        dst[0] = '\0';
        return;
    }

    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
}

static const char *ring_operations_content(void) {
    static char out[768];
    int pos = 0;
    int start = ring_op_count - RING_OP_COUNT;

    if (start < 0) {
        start = 0;
    }

    out[0] = '\0';

    if (ring_op_count == 0) {
        ring_copy_text(out, "No AI operations yet", 768);
        return out;
    }

    for (int i = start; i < ring_op_count; i++) {
        int slot = i % RING_OP_COUNT;
        const char *prefix = "- ";

        for (int x = 0; prefix[x] && pos < 767; x++) {
            out[pos++] = prefix[x];
        }

        for (int x = 0; ring_ops[slot][x] && pos < 767; x++) {
            char c = ring_ops[slot][x];

            if (c == '\n' || c == '\r') {
                c = ' ';
            }

            out[pos++] = c;
        }

        if (i != ring_op_count - 1 && pos < 766) {
            out[pos++] = '\\';
            out[pos++] = 'n';
        }
    }

    out[pos] = '\0';
    return out;
}

static void ring_update_home_operations(void) {
    if (workspace_builder_replace_block(
            "/workspaces/home.workspace",
            "AI Operations",
            "logs",
            "AI Operations",
            ring_operations_content()
        )) {
        return;
    }

    workspace_builder_replace_block(
        "/workspaces/home.workspace",
        "Task List",
        "logs",
        "AI Operations",
        ring_operations_content()
    );
}

void ring_log_operation(const char *text) {
    int slot = ring_op_count % RING_OP_COUNT;

    ring_copy_text(ring_ops[slot], text ? text : "operation", RING_OP_LEN);
    ring_op_count++;

    ring_update_home_operations();
}

void ring_show_operations(void) {
    kprintf("AI Operations\n");
    kprintf("  %s\n", ring_operations_content());
}


static const char *ring_home_content(void) {
    if (ring_state == RING_IDLE) {
        return "State: idle\\nVisual: centered input bar\\nWaiting for the first thought";
    }

    if (ring_state == RING_CHAT) {
        return "State: chat\\nVisual: full-screen conversation\\nUser is typing or speaking";
    }

    if (ring_state == RING_LISTENING) {
        return "State: listening\\nVisual: pulsing ring\\nVoice capture active";
    }

    if (ring_state == RING_THINKING) {
        return "State: thinking\\nVisual: vibrating ring\\nAI is preparing workspace mutation";
    }

    if (ring_state == RING_DOCKED) {
        return "State: docked\\nVisual: ring at screen edge\\nWorkspace is ready";
    }

    return "State: unknown";
}

static void ring_update_chat_visual(void) {
    if (workspace_builder_replace_block(
            "/workspaces/chat.workspace",
            "AI Ring",
            "status",
            "AI Ring",
            ring_home_content()
        )) {
        return;
    }

    service_call("workspace", "template", "chat /workspaces/chat.workspace");

    workspace_builder_replace_block(
        "/workspaces/chat.workspace",
        "AI Ring",
        "status",
        "AI Ring",
        ring_home_content()
    );
}

static void ring_update_home_visual(void) {
    if (workspace_builder_replace_block(
            "/workspaces/home.workspace",
            "AI Ring",
            "ai",
            "AI Ring",
            ring_home_content()
        )) {
        return;
    }

    service_call("workspace", "template", "home /workspaces/home.workspace");

    workspace_builder_replace_block(
        "/workspaces/home.workspace",
        "AI Ring",
        "ai",
        "AI Ring",
        ring_home_content()
    );
}

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
    ring_op_count = 0;
    eventlog_add("ai ring initialized");
}

void ring_set_state(const char *state) {
    if (!state) return;

    if (strcmp(state, "idle") == 0) {
        ring_state = RING_IDLE;
        eventlog_add("ring state idle");
        ring_update_home_visual();
        ring_update_chat_visual();
        ring_log_operation("ring: idle");
        return;
    }

    if (strcmp(state, "chat") == 0 || strcmp(state, "open") == 0) {
        ring_state = RING_CHAT;
        eventlog_add("ring state chat");
        ring_update_home_visual();
        ring_update_chat_visual();
        ring_log_operation("ring: chat");
        return;
    }

    if (strcmp(state, "listen") == 0 || strcmp(state, "listening") == 0) {
        ring_state = RING_LISTENING;
        eventlog_add("ring state listening");
        ring_update_home_visual();
        ring_update_chat_visual();
        ring_log_operation("ring: listening");
        return;
    }

    if (strcmp(state, "think") == 0 || strcmp(state, "thinking") == 0) {
        ring_state = RING_THINKING;
        eventlog_add("ring state thinking");
        ring_update_home_visual();
        ring_update_chat_visual();
        ring_log_operation("ring: thinking");
        return;
    }

    if (strcmp(state, "dock") == 0 || strcmp(state, "docked") == 0) {
        ring_state = RING_DOCKED;
        eventlog_add("ring state docked");
        ring_update_home_visual();
        ring_update_chat_visual();
        ring_log_operation("ring: docked");
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

    if (strcmp(command, "home") == 0) {
        ring_update_home_visual();

        if (workspace_builder_open("/workspaces/home.workspace")) {
            return 1;
        }

        if (workspace_builder_open("home.workspace")) {
            return 1;
        }

        kprintf("Ring: home open failed\n");
        return 0;
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
    ring_log_operation(text);
    kprintf("Ring: chat expanded\n");

    ring_set_state("thinking");
    kprintf("AI: %s\n", text);
    kprintf("Ring: thinking\n");

    ring_set_state("docked");
    kprintf("Ring: docked\n");

    if (!intent_handle(text)) {
        kprintf("AI: intent failed\n");
        ring_set_state("chat");
        return 0;
    }

    return 1;
}
