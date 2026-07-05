#include "../include/host_bridge.h"
#include "../include/serial.h"
#include "../include/gfx.h"
#include "../include/string.h"

#define HOST_BRIDGE_LINE_MAX 256

static char host_bridge_line[HOST_BRIDGE_LINE_MAX];
static int host_bridge_line_len = 0;

/*
 * 0 = AI surface
 * 1 = workspace surface
 */
static int host_bridge_surface = 0;

static void host_bridge_redraw_surface(void) {
    if (host_bridge_surface == 1) {
        gfx_draw_workspace_surface();
    } else {
        gfx_draw_ai_surface();
    }
}

static void host_bridge_apply_state(const char *state) {
    if (!state) {
        return;
    }

    if (strcmp(state, "idle") == 0 || strcmp(state, "offline") == 0) {
        gfx_set_model_state(0);
        host_bridge_redraw_surface();
        return;
    }

    if (strcmp(state, "loading") == 0 || strcmp(state, "load") == 0) {
        gfx_set_model_state(1);
        host_bridge_redraw_surface();
        return;
    }

    if (strcmp(state, "ready") == 0) {
        gfx_set_model_state(2);
        host_bridge_redraw_surface();
        return;
    }

    if (strcmp(state, "thinking") == 0 || strcmp(state, "think") == 0) {
        gfx_set_model_state(3);
        host_bridge_redraw_surface();
        return;
    }

    if (strcmp(state, "drawing") == 0 || strcmp(state, "drawing-ui") == 0 || strcmp(state, "draw") == 0) {
        gfx_set_model_state(4);
        host_bridge_redraw_surface();
        return;
    }
}

static void host_bridge_apply_surface(const char *surface) {
    if (!surface) {
        return;
    }

    if (strcmp(surface, "ai") == 0 || strcmp(surface, "surface") == 0) {
        host_bridge_surface = 0;
        gfx_draw_ai_surface();
        return;
    }

    if (strcmp(surface, "workspace") == 0 || strcmp(surface, "ws") == 0) {
        host_bridge_surface = 1;
        gfx_draw_workspace_surface();
        return;
    }
}

static void host_bridge_handle_line(const char *line) {
    const char *payload;

    if (!line) {
        return;
    }

    /*
     * Host -> LOS protocol:
     *
     * LOS_UI|state|ready
     * LOS_UI|state|thinking
     * LOS_UI|surface|workspace
     * LOS_UI|surface|ai
     */
    if (strncmp(line, "LOS_UI|state|", 13) == 0) {
        payload = line + 13;
        host_bridge_apply_state(payload);
        return;
    }

    if (strncmp(line, "LOS_UI|surface|", 15) == 0) {
        payload = line + 15;
        host_bridge_apply_surface(payload);
        return;
    }

    if (strncmp(line, "LOS_UI|note|", 12) == 0) {
        payload = line + 12;
        gfx_draw_host_note(payload);
        return;
    }

}

void host_bridge_poll(void) {
    char c;
    int guard = 0;

    /*
     * Read a small batch per kernel tick so we do not block input/rendering.
     */
    while (guard < 64 && serial_read_char_nonblocking(&c)) {
        guard++;

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            host_bridge_line[host_bridge_line_len] = '\0';
            host_bridge_handle_line(host_bridge_line);
            host_bridge_line_len = 0;
            host_bridge_line[0] = '\0';
            continue;
        }

        if (host_bridge_line_len < HOST_BRIDGE_LINE_MAX - 1) {
            host_bridge_line[host_bridge_line_len++] = c;
        } else {
            host_bridge_line_len = 0;
            host_bridge_line[0] = '\0';
        }
    }
}
