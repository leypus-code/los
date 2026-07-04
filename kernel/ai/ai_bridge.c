#include "../include/ai_bridge.h"
#include "../include/serial.h"
#include "../include/kprintf.h"
#include "../include/string.h"
#include "../include/intent.h"
#include "../include/ring.h"
#include "../include/workspace_builder.h"
#include "../include/service.h"

static int ai_bridge_starts_with(const char *s, const char *prefix) {
    int i = 0;

    if (!s || !prefix) return 0;

    while (prefix[i]) {
        if (s[i] != prefix[i]) {
            return 0;
        }
        i++;
    }

    return 1;
}

static void ai_bridge_send_prompt(const char *prompt) {
    serial_write_string("LOS_AI_REQUEST:");
    serial_write_string(prompt);
    serial_write_string("\n");
}

static int ai_bridge_read_line(char *out, int max) {
    int pos = 0;
    unsigned int idle = 0;

    if (!out || max <= 0) return 0;

    out[0] = '\0';

    while (idle < 250000000) {
        char c;

        if (serial_read_char_nonblocking(&c)) {
            idle = 0;

            if (c == '\r') {
                continue;
            }

            if (c == '\n') {
                out[pos] = '\0';
                return pos > 0;
            }

            if (pos < max - 1) {
                out[pos++] = c;
            }
        } else {
            idle++;
        }
    }

    out[pos] = '\0';
    return pos > 0;
}

static int ai_bridge_strip_named_prefix(char *line, const char *prefix, char *out, int max) {
    int i = 0;
    int p = 0;

    if (!line || !prefix || !out || max <= 0) return 0;

    while (prefix[i]) {
        if (line[i] != prefix[i]) {
            return 0;
        }
        i++;
    }

    while (line[i] && p < max - 1) {
        out[p++] = line[i++];
    }

    out[p] = '\0';
    return 1;
}

static int ai_bridge_strip_prefix(char *line, char *out, int max) {
    return ai_bridge_strip_named_prefix(line, "LOS_AI_RESPONSE:", out, max);
}

static void ai_bridge_update_web_widget(const char *query, const char *answer) {
    char content[512];
    int pos = 0;

    if (!query) query = "";
    if (!answer) answer = "";

    const char *q = "Query: ";
    const char *sep = "\\nResult: ";

    for (int i = 0; q[i] && pos < 511; i++) {
        content[pos++] = q[i];
    }

    for (int i = 0; query[i] && pos < 511; i++) {
        char c = query[i];
        if (c == '\n' || c == '\r') c = ' ';
        if (c == '|') c = '/';
        content[pos++] = c;
    }

    for (int i = 0; sep[i] && pos < 511; i++) {
        content[pos++] = sep[i];
    }

    for (int i = 0; answer[i] && pos < 511; i++) {
        char c = answer[i];
        if (c == '\n' || c == '\r') c = ' ';
        if (c == '|') c = '/';
        content[pos++] = c;
    }

    content[pos] = '\0';

    service_call("workspace", "template", "home /workspaces/home.workspace");

    if (workspace_builder_replace_block(
            "/workspaces/home.workspace",
            "Web Result",
            "text",
            "Web Result",
            content
        )) {
        return;
    }

    if (workspace_builder_replace_block(
            "/workspaces/home.workspace",
            "Command Center",
            "text",
            "Web Result",
            content
        )) {
        return;
    }

    if (workspace_builder_replace_block(
            "/workspaces/home.workspace",
            "Weather",
            "text",
            "Web Result",
            content
        )) {
        return;
    }

    if (workspace_builder_replace_block(
            "/workspaces/home.workspace",
            "Code Workspace",
            "text",
            "Web Result",
            content
        )) {
        return;
    }

    workspace_builder_replace_block(
        "/workspaces/home.workspace",
        "Blank Canvas",
        "text",
        "Web Result",
        content
    );
}

static void ai_bridge_open_home_after_web(void) {
    if (workspace_builder_open("/workspaces/home.workspace")) {
        return;
    }

    workspace_builder_open("home.workspace");
}


int ai_bridge_ask(const char *prompt, char *out, int max) {
    char line[384];

    if (!prompt || !prompt[0] || !out || max <= 0) {
        return 0;
    }

    out[0] = '\0';

    if (!serial_is_ready()) {
        kprintf("AI Bridge: serial not ready\n");
        return 0;
    }

    ai_bridge_send_prompt(prompt);
    kprintf("AI Bridge: waiting for host response...\n");

    if (!ai_bridge_read_line(line, 384)) {
        kprintf("AI Bridge: no response\n");
        return 0;
    }

    if (!ai_bridge_strip_prefix(line, out, max)) {
        kprintf("AI Bridge: bad response\n");
        kprintf("%s\n", line);
        return 0;
    }

    return 1;
}

int ai_bridge_execute(const char *prompt) {
    char answer[256];

    if (!ai_bridge_ask(prompt, answer, 256)) {
        kprintf("AI Bridge: fallback to local chat\n");
        return ring_chat(prompt);
    }

    kprintf("AI Bridge: %s\n", answer);
    ring_log_operation("model: response received");

    if (ai_bridge_starts_with(answer, "web:")) {
        ring_log_operation("model: selected web tool");
        return ai_bridge_web(answer + 4);
    }

    if (!intent_handle(answer)) {
        kprintf("AI Bridge: response was not an intent, showing as operation\n");
        ring_log_operation(answer);
        return 1;
    }

    return 1;
}

int ai_bridge_web(const char *query) {
    char line[512];
    char answer[384];

    if (!query || !query[0]) {
        kprintf("Web: empty query\n");
        return 0;
    }

    if (!serial_is_ready()) {
        kprintf("Web Bridge: serial not ready\n");
        return 0;
    }

    serial_write_string("LOS_WEB_REQUEST:");
    serial_write_string(query);
    serial_write_string("\n");
    kprintf("Web Bridge: waiting for host response...\n");

    if (!ai_bridge_read_line(line, 512)) {
        kprintf("Web Bridge: no response\n");
        return 0;
    }

    if (!ai_bridge_strip_named_prefix(line, "LOS_WEB_RESPONSE:", answer, 384)) {
        kprintf("Web Bridge: bad response\n");
        kprintf("%s\n", line);
        return 0;
    }

    kprintf("Web: %s\n", answer);
    ring_log_operation("web: response received");
    ai_bridge_update_web_widget(query, answer);
    ai_bridge_open_home_after_web();
    return 1;
}
