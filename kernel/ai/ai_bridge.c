#include "../include/ai_bridge.h"
#include "../include/serial.h"
#include "../include/kprintf.h"
#include "../include/string.h"
#include "../include/intent.h"
#include "../include/ring.h"

static int ai_bridge_starts_with(const char *s, const char *prefix);
static void ai_bridge_send_prompt(const char *prompt);
static int ai_bridge_read_line(char *out, int max);
static int ai_bridge_strip_named_prefix(char *line, const char *prefix, char *out, int max);
static int ai_bridge_strip_prefix(char *line, char *out, int max);
static void ai_bridge_update_web_widget(const char *query, const char *answer);
static void ai_bridge_update_chat_widget(const char *query, const char *answer);
static void ai_bridge_open_chat_after_result(void);

#include "../include/workspace_builder.h"
#include "../include/service.h"


#define CHAT_TRANSCRIPT_COUNT 8
#define CHAT_TRANSCRIPT_LEN   128

static char chat_transcript[CHAT_TRANSCRIPT_COUNT][CHAT_TRANSCRIPT_LEN];
static int chat_transcript_count = 0;
static char chat_last_user[96];

static void ai_bridge_copy_safe(char *dst, const char *src, int max) {
    int i = 0;

    if (!dst || max <= 0) return;

    if (!src) {
        dst[0] = '\0';
        return;
    }

    while (src[i] && i < max - 1) {
        char c = src[i];

        if (c == '\n' || c == '\r') c = ' ';
        if (c == '|') c = '/';

        dst[i] = c;
        i++;
    }

    dst[i] = '\0';
}

static void ai_bridge_chat_add_line(const char *prefix, const char *text) {
    int slot = chat_transcript_count % CHAT_TRANSCRIPT_COUNT;
    int pos = 0;

    if (!prefix) prefix = "";
    if (!text) text = "";

    for (int i = 0; prefix[i] && pos < CHAT_TRANSCRIPT_LEN - 1; i++) {
        chat_transcript[slot][pos++] = prefix[i];
    }

    for (int i = 0; text[i] && pos < CHAT_TRANSCRIPT_LEN - 1; i++) {
        char c = text[i];

        if (c == '\n' || c == '\r') c = ' ';
        if (c == '|') c = '/';

        chat_transcript[slot][pos++] = c;
    }

    chat_transcript[slot][pos] = '\0';
    chat_transcript_count++;
}

static const char *ai_bridge_chat_transcript_content(void) {
    static char out[768];
    int pos = 0;
    int start = chat_transcript_count - CHAT_TRANSCRIPT_COUNT;

    if (start < 0) start = 0;

    if (chat_transcript_count == 0) {
        ai_bridge_copy_safe(out, "No conversation yet.\\nUse talk \"what is docker\"", 768);
        return out;
    }

    for (int i = start; i < chat_transcript_count; i++) {
        int slot = i % CHAT_TRANSCRIPT_COUNT;

        for (int x = 0; chat_transcript[slot][x] && pos < 767; x++) {
            out[pos++] = chat_transcript[slot][x];
        }

        if (i != chat_transcript_count - 1 && pos < 766) {
            out[pos++] = '\\';
            out[pos++] = 'n';
        }
    }

    out[pos] = '\0';
    return out;
}

static int ai_bridge_chat_last_is_thinking(void) {
    if (chat_transcript_count <= 0) {
        return 0;
    }

    int slot = (chat_transcript_count - 1) % CHAT_TRANSCRIPT_COUNT;

    return strcmp(chat_transcript[slot], "AI: thinking...") == 0;
}

static void ai_bridge_chat_replace_last_ai(const char *answer) {
    int slot = 0;
    int pos = 0;
    const char *prefix = "AI: ";

    if (chat_transcript_count <= 0) {
        return;
    }

    if (!answer) {
        answer = "";
    }

    slot = (chat_transcript_count - 1) % CHAT_TRANSCRIPT_COUNT;

    for (int i = 0; prefix[i] && pos < CHAT_TRANSCRIPT_LEN - 1; i++) {
        chat_transcript[slot][pos++] = prefix[i];
    }

    for (int i = 0; answer[i] && pos < CHAT_TRANSCRIPT_LEN - 1; i++) {
        char c = answer[i];

        if (c == '\n' || c == '\r') c = ' ';
        if (c == '|') c = '/';

        chat_transcript[slot][pos++] = c;
    }

    chat_transcript[slot][pos] = '\0';
}

static void ai_bridge_chat_remember(const char *query, const char *answer) {
    if (!query) query = "";
    if (!answer) answer = "";

    if (strcmp(answer, "thinking...") == 0) {
        ai_bridge_chat_add_line("User: ", query);
        ai_bridge_chat_add_line("AI: ", "thinking...");
        ai_bridge_copy_safe(chat_last_user, query, 96);
        return;
    }

    /*
     * If the previous line is AI: thinking... for the same user input,
     * replace it with the final AI answer instead of appending another AI line.
     */
    if (strcmp(chat_last_user, query) == 0 && ai_bridge_chat_last_is_thinking()) {
        ai_bridge_chat_replace_last_ai(answer);
        return;
    }

    if (strcmp(chat_last_user, query) != 0) {
        ai_bridge_chat_add_line("User: ", query);
        ai_bridge_copy_safe(chat_last_user, query, 96);
    }

    ai_bridge_chat_add_line("AI: ", answer);
}


static void ai_bridge_update_chat_widget(const char *query, const char *answer) {
    char content[512];
    char input_content[256];
    int pos = 0;
    int ipos = 0;
    int updated = 0;

    if (!query) query = "";
    if (!answer) answer = "";

    ai_bridge_chat_remember(query, answer);

    /*
     * Compact one-line result for the small Last Tool Result block.
     */
    for (int i = 0; query[i] && pos < 511; i++) {
        char c = query[i];

        if (c == '\n' || c == '\r') c = ' ';
        if (c == '|') c = '/';

        content[pos++] = c;
    }

    if (pos < 508) {
        content[pos++] = ' ';
        content[pos++] = '=';
        content[pos++] = '>';
        content[pos++] = ' ';
    }

    for (int i = 0; answer[i] && pos < 511; i++) {
        char c = answer[i];

        if (c == '\n' || c == '\r') c = ' ';
        if (c == '|') c = '/';

        content[pos++] = c;
    }

    content[pos] = '\0';

    /*
     * Center Input shows current/last typed phrase.
     */
    const char *input_prefix = "Last input: ";
    for (int i = 0; input_prefix[i] && ipos < 255; i++) {
        input_content[ipos++] = input_prefix[i];
    }

    for (int i = 0; query[i] && ipos < 255; i++) {
        char c = query[i];

        if (c == '\n' || c == '\r') c = ' ';
        if (c == '|') c = '/';

        input_content[ipos++] = c;
    }

    input_content[ipos] = '\0';

    if (!workspace_builder_replace_block(
            "/workspaces/chat.workspace",
            "Last Tool Result",
            "text",
            "Last Tool Result",
            content
        )) {
        service_call("workspace", "template", "chat /workspaces/chat.workspace");

        updated = workspace_builder_replace_block(
            "/workspaces/chat.workspace",
            "Last Tool Result",
            "text",
            "Last Tool Result",
            content
        );
    } else {
        updated = 1;
    }

    if (!updated) {
        updated = workspace_builder_replace_block(
            "/workspaces/chat.workspace",
            "Tool Result",
            "text",
            "Last Tool Result",
            content
        );
    }

    if (!updated) {
        updated = workspace_builder_replace_block(
            "/workspaces/chat.workspace",
            "Web Result",
            "text",
            "Last Tool Result",
            content
        );
    }

    workspace_builder_replace_block(
        "/workspaces/chat.workspace",
        "Center Input",
        "ai",
        "Center Input",
        input_content
    );

    workspace_builder_replace_block(
        "/workspaces/chat.workspace",
        "Conversation",
        "text",
        "Conversation",
        ai_bridge_chat_transcript_content()
    );

    if (updated) {
        kprintf("Chat Screen: result updated\n");
        ring_log_operation("chat screen: result updated");
    } else {
        kprintf("Chat Screen: result update failed\n");
        ring_log_operation("chat screen: result update failed");
    }
}

static void ai_bridge_open_chat_after_result(void) {
    if (workspace_builder_open("/workspaces/chat.workspace")) {
        return;
    }

    workspace_builder_open("chat.workspace");
}




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

    for (int i = 0; query[i] && pos < 511; i++) {
        char c = query[i];

        if (c == '\n' || c == '\r') c = ' ';
        if (c == '|') c = '/';

        content[pos++] = c;
    }

    if (pos < 508) {
        content[pos++] = ' ';
        content[pos++] = '=';
        content[pos++] = '>';
        content[pos++] = ' ';
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




static int ai_bridge_use_host = 0;

static char ai_context_workspace[64] = "Chat Screen";
static char ai_context_last_intent[96] = "none";
static char ai_context_last_answer[160] = "none";

void ai_bridge_context_set_workspace(const char *name) {
    ai_bridge_copy_safe(ai_context_workspace, name ? name : "unknown", 64);
}

void ai_bridge_context_set_last_intent(const char *intent) {
    ai_bridge_copy_safe(ai_context_last_intent, intent ? intent : "none", 96);
}

void ai_bridge_context_show(void) {
    kprintf("AI Context\n");
    kprintf("Workspace: %s\n", ai_context_workspace);
    kprintf("Host bridge: %s\n", ai_bridge_use_host ? "on" : "off");
    kprintf("AI input mode: shell-owned\n");
    kprintf("Last intent: %s\n", ai_context_last_intent);
    kprintf("Last answer: %s\n", ai_context_last_answer);
}

static void ai_bridge_context_set_answer(const char *answer) {
    ai_bridge_copy_safe(ai_context_last_answer, answer ? answer : "none", 160);
}


static void ai_bridge_append(char *dst, const char *src, int max) {
    int pos = 0;
    int i = 0;

    if (!dst || !src || max <= 0) {
        return;
    }

    while (dst[pos] && pos < max - 1) {
        pos++;
    }

    while (src[i] && pos < max - 1) {
        dst[pos++] = src[i++];
    }

    dst[pos] = '\0';
}


void ai_bridge_set_host_enabled(int enabled) {
    ai_bridge_use_host = enabled ? 1 : 0;
}

int ai_bridge_host_enabled(void) {
    return ai_bridge_use_host;
}

void ai_bridge_status(void) {
    kprintf("AI Bridge host mode: %s\n", ai_bridge_use_host ? "on" : "off");
    kprintf("Local embedded AI: on\n");
}


void ai_bridge_model_status(void) {
    kprintf("AI Provider\n");
    kprintf("Current: %s\n", ai_bridge_use_host ? "host" : "local");
    kprintf("Contract: v1\n");
    kprintf("Local fallback: on\n");
    kprintf("Host bridge: %s\n", ai_bridge_use_host ? "on" : "off");
    kprintf("Outputs: answer, intent, web, ui_patch later\n");
}

void ai_bridge_model_set_local(void) {
    ai_bridge_set_host_enabled(0);
    kprintf("[OK] AI provider set to local\n");
}

void ai_bridge_model_set_host(void) {
    ai_bridge_set_host_enabled(1);
    kprintf("[OK] AI provider set to host\n");
}

void ai_bridge_packet_show(void) {
    kprintf("AI Request Packet v1\n");
    kprintf("workspace=%s\n", ai_context_workspace);
    kprintf("provider=%s\n", ai_bridge_use_host ? "host" : "local");
    kprintf("host_bridge=%s\n", ai_bridge_use_host ? "on" : "off");
    kprintf("last_intent=%s\n", ai_context_last_intent);
    kprintf("last_answer=%s\n", ai_context_last_answer);
    kprintf("available_outputs=answer,intent,web,ui_patch\n");
    kprintf("available_intents=home,build dashboard,coding mode,blank canvas,reset home,debug build error,write notes,plan project\n");
}

static char ai_bridge_ascii_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + 32);
    }

    return c;
}

static int ai_bridge_contains_ci(const char *text, const char *needle) {
    int i = 0;

    if (!text || !needle || !needle[0]) return 0;

    while (text[i]) {
        int j = 0;

        while (needle[j]) {
            char a = ai_bridge_ascii_lower(text[i + j]);
            char b = ai_bridge_ascii_lower(needle[j]);

            if (!text[i + j] || a != b) {
                break;
            }

            j++;
        }

        if (!needle[j]) {
            return 1;
        }

        i++;
    }

    return 0;
}

static int ai_bridge_local_answer(const char *prompt, const char *answer) {
    kprintf("Local AI: %s\n", answer);
    ring_log_operation("local model: answer");
    ai_bridge_context_set_last_intent("chat answer");
    ai_bridge_context_set_answer(answer);
    ai_bridge_update_chat_widget(prompt, answer);
    ai_bridge_open_chat_after_result();
    return 1;
}

static int ai_bridge_local_intent(const char *prompt, const char *intent) {
    kprintf("Local AI: %s\n", intent);
    ring_log_operation("local model: intent");
    ai_bridge_context_set_last_intent(intent);
    ai_bridge_context_set_answer(intent);
    ai_bridge_update_chat_widget(prompt, intent);

    if (!intent_handle(intent)) {
        return ai_bridge_local_answer(prompt, "Local model selected an intent, but LOS could not execute it.");
    }

    return 1;
}

static int ai_bridge_local_execute(const char *prompt) {
    if (!prompt || !prompt[0]) {
        return 0;
    }

    /*
     * Tiny embedded default model.
     * This is intentionally simple: no heap, no network, no external API.
     * It gives LOS useful behavior immediately after boot.
     */

    if (ai_bridge_contains_ci(prompt, "dashboard") ||
        ai_bridge_contains_ci(prompt, "dash") ||
        ai_bridge_contains_ci(prompt, "main screen")) {
        return ai_bridge_local_intent(prompt, "build dashboard");
    }

    if (ai_bridge_contains_ci(prompt, "coding") ||
        ai_bridge_contains_ci(prompt, "code mode") ||
        ai_bridge_contains_ci(prompt, "developer") ||
        ai_bridge_contains_ci(prompt, "kernel build")) {
        return ai_bridge_local_intent(prompt, "coding mode");
    }

    if (ai_bridge_contains_ci(prompt, "blank") ||
        ai_bridge_contains_ci(prompt, "empty") ||
        ai_bridge_contains_ci(prompt, "canvas")) {
        return ai_bridge_local_intent(prompt, "blank canvas");
    }

    if (ai_bridge_contains_ci(prompt, "reset")) {
        return ai_bridge_local_intent(prompt, "reset home");
    }

    if (ai_bridge_contains_ci(prompt, "debug") ||
        ai_bridge_contains_ci(prompt, "build error") ||
        ai_bridge_contains_ci(prompt, "fix build")) {
        return ai_bridge_local_intent(prompt, "debug build error");
    }

    if (ai_bridge_contains_ci(prompt, "note") ||
        ai_bridge_contains_ci(prompt, "notes")) {
        return ai_bridge_local_intent(prompt, "write notes");
    }

    if (ai_bridge_contains_ci(prompt, "plan") ||
        ai_bridge_contains_ci(prompt, "planning")) {
        return ai_bridge_local_intent(prompt, "plan project");
    }

    if (ai_bridge_contains_ci(prompt, "checklist") ||
        ai_bridge_contains_ci(prompt, "todo")) {
        return ai_bridge_local_intent(prompt, "add checklist");
    }

    if (ai_bridge_contains_ci(prompt, "logs") ||
        ai_bridge_contains_ci(prompt, "log panel")) {
        return ai_bridge_local_intent(prompt, "add logs panel");
    }

    if (ai_bridge_contains_ci(prompt, "weather")) {
        return ai_bridge_local_answer(
            prompt,
            "Local mode has no live weather. Enable host bridge for internet weather, or use add weather widget."
        );
    }

    if (ai_bridge_contains_ci(prompt, "docker")) {
        return ai_bridge_local_answer(
            prompt,
            "Docker is a platform for packaging and running applications in isolated containers."
        );
    }

    if (ai_bridge_contains_ci(prompt, "linux")) {
        return ai_bridge_local_answer(
            prompt,
            "Linux is an open-source Unix-like operating system kernel used by many distributions."
        );
    }

    if (ai_bridge_contains_ci(prompt, "los")) {
        return ai_bridge_local_answer(
            prompt,
            "LOS is an AI-native OS prototype where chat, tasks, tools, and mutable workspaces are first-class system concepts."
        );
    }

    if (ai_bridge_contains_ci(prompt, "where am i") ||
        ai_bridge_contains_ci(prompt, "current screen") ||
        ai_bridge_contains_ci(prompt, "current workspace")) {
        char msg[220];

        msg[0] = '\0';
        ai_bridge_append(msg, "You are in ", 220);
        ai_bridge_append(msg, ai_context_workspace, 220);
        ai_bridge_append(msg, ". Host bridge is ", 220);
        ai_bridge_append(msg, ai_bridge_use_host ? "on" : "off", 220);
        ai_bridge_append(msg, ". Last intent: ", 220);
        ai_bridge_append(msg, ai_context_last_intent, 220);
        ai_bridge_append(msg, ".", 220);

        return ai_bridge_local_answer(prompt, msg);
    }

    if (ai_bridge_contains_ci(prompt, "what can i do") ||
        ai_bridge_contains_ci(prompt, "help") ||
        ai_bridge_contains_ci(prompt, "commands")) {
        return ai_bridge_local_answer(
            prompt,
            "You can type naturally: make dashboard, coding mode, blank canvas, debug build, notes, plan project, what is docker, or bridge on for web tools."
        );
    }

    if (ai_bridge_contains_ci(prompt, "what happened") ||
        ai_bridge_contains_ci(prompt, "last action") ||
        ai_bridge_contains_ci(prompt, "status")) {
        char msg[240];

        msg[0] = '\0';
        ai_bridge_append(msg, "Last intent: ", 240);
        ai_bridge_append(msg, ai_context_last_intent, 240);
        ai_bridge_append(msg, ". Last answer: ", 240);
        ai_bridge_append(msg, ai_context_last_answer, 240);

        return ai_bridge_local_answer(prompt, msg);
    }

    return ai_bridge_local_answer(
        prompt,
        "Local AI did not understand fully. Try dashboard, coding mode, debug build, notes, plan, docker, linux, or bridge on."
    );
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

    if (!prompt || !prompt[0]) {
        return 0;
    }

    /*
     * Default MVP behavior:
     * Use embedded local AI immediately.
     * Host bridge is optional and must be enabled with `bridge on`.
     */
    if (!ai_bridge_use_host) {
        return ai_bridge_local_execute(prompt);
    }

    if (!ai_bridge_ask(prompt, answer, 256)) {
        kprintf("AI Bridge: host unavailable, using local AI\n");
        return ai_bridge_local_execute(prompt);
    }

    kprintf("AI Bridge: %s\n", answer);
    ring_log_operation("model: response received");

    if (ai_bridge_starts_with(answer, "web:")) {
        ring_log_operation("model: selected web tool");
        return ai_bridge_web(answer + 4);
    }

    ai_bridge_update_chat_widget(prompt, answer);

    if (!intent_handle(answer)) {
        kprintf("AI Bridge: response was not an intent, showing as chat answer\n");
        ai_bridge_update_chat_widget(prompt, answer);
        ai_bridge_open_chat_after_result();
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
    ai_bridge_update_chat_widget(query, answer);
    ai_bridge_open_chat_after_result();
    return 1;
}

int ai_bridge_talk(const char *text) {
    if (!text || !text[0]) {
        kprintf("Talk: empty\n");
        return 0;
    }

    /*
     * Important:
     * Do not open Chat Screen before bridge/model execution.
     * workspace_builder_open() is modal and blocks until Q.
     * So we update the chat workspace first, execute AI/tool routing,
     * then the web/intent path opens the final screen.
     */
    ring_set_state("chat");
    ring_log_operation("talk: user input");

    ai_bridge_update_chat_widget(text, "thinking...");

    ring_set_state("thinking");
    kprintf("Talk: %s\n", text);

    if (!ai_bridge_execute(text)) {
        ring_set_state("chat");
        return 0;
    }

    ring_set_state("docked");
    return 1;
}



void ai_bridge_show_transcript(void) {
    int start = chat_transcript_count - CHAT_TRANSCRIPT_COUNT;

    if (start < 0) {
        start = 0;
    }

    kprintf("Chat Transcript\n");

    if (chat_transcript_count == 0) {
        kprintf("No conversation yet.\n");
        return;
    }

    for (int i = start; i < chat_transcript_count; i++) {
        int slot = i % CHAT_TRANSCRIPT_COUNT;
        kprintf("%s\n", chat_transcript[slot]);
    }
}

