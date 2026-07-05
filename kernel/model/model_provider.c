#include "../include/model_provider.h"
#include "../include/gfx.h"
#include "../include/kprintf.h"
#include "../include/string.h"
#include "../include/serial.h"

static model_provider_mode_t provider_mode = MODEL_PROVIDER_OFFLINE;
static model_status_t provider_status = MODEL_STATUS_OFFLINE;

static char loaded_model_name[64];

static void model_provider_send_packet(const char *kind, const char *payload) {
    if (!serial_is_ready()) {
        return;
    }

    serial_write_string("LOS_AI|");
    serial_write_string(kind ? kind : "unknown");
    serial_write_string("|");
    serial_write_line(payload ? payload : "");
}


void model_provider_initialize(void) {
    provider_mode = MODEL_PROVIDER_OFFLINE;
    provider_status = MODEL_STATUS_OFFLINE;
    loaded_model_name[0] = '\0';

    gfx_set_model_state(0);
}

void model_provider_set_mode(model_provider_mode_t mode) {
    provider_mode = mode;

    if (mode == MODEL_PROVIDER_OFFLINE) {
        provider_status = MODEL_STATUS_OFFLINE;
        gfx_set_model_state(0);
        model_provider_send_packet("mode", "offline");
        return;
    }

    provider_status = MODEL_STATUS_READY;
    gfx_set_model_state(2);
    model_provider_send_packet("mode", model_provider_mode_name());
}

model_provider_mode_t model_provider_get_mode(void) {
    return provider_mode;
}

void model_provider_set_status(model_status_t status) {
    provider_status = status;

    if (status == MODEL_STATUS_OFFLINE) {
        gfx_set_model_state(0);
    } else if (status == MODEL_STATUS_LOADING) {
        gfx_set_model_state(1);
    } else if (status == MODEL_STATUS_READY) {
        gfx_set_model_state(2);
    } else if (status == MODEL_STATUS_THINKING) {
        gfx_set_model_state(3);
    } else if (status == MODEL_STATUS_DRAWING_UI) {
        gfx_set_model_state(4);
    }
}

model_status_t model_provider_get_status(void) {
    return provider_status;
}

const char *model_provider_mode_name(void) {
    if (provider_mode == MODEL_PROVIDER_HOST) return "host";
    if (provider_mode == MODEL_PROVIDER_LOCAL) return "local";
    return "offline";
}

const char *model_provider_status_name(void) {
    if (provider_status == MODEL_STATUS_LOADING) return "loading";
    if (provider_status == MODEL_STATUS_READY) return "ready";
    if (provider_status == MODEL_STATUS_THINKING) return "thinking";
    if (provider_status == MODEL_STATUS_DRAWING_UI) return "drawing-ui";
    return "offline";
}

void model_provider_load(const char *model_name) {
    int i = 0;

    if (!model_name || !model_name[0]) {
        model_name = "llama";
    }

    while (model_name[i] && i < 63) {
        loaded_model_name[i] = model_name[i];
        i++;
    }

    loaded_model_name[i] = '\0';

    /*
     * v24.5:
     * request host bridge to load model.
     */
    provider_status = MODEL_STATUS_LOADING;
    gfx_set_model_state(1);
    model_provider_send_packet("load", loaded_model_name);
}

void model_provider_ask(const char *prompt) {
    (void)prompt;

    /*
     * v24.4 stub:
     * for now we only switch visual lifecycle state.
     * v24.5 will send prompt to host bridge.
     */
    if (provider_mode == MODEL_PROVIDER_OFFLINE) {
        provider_status = MODEL_STATUS_OFFLINE;
        gfx_set_model_state(0);
        return;
    }

    provider_status = MODEL_STATUS_THINKING;
    gfx_set_model_state(3);
    model_provider_send_packet("ask", prompt);
}
