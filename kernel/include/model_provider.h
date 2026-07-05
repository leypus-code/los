#ifndef LOS_MODEL_PROVIDER_H
#define LOS_MODEL_PROVIDER_H

typedef enum {
    MODEL_PROVIDER_OFFLINE = 0,
    MODEL_PROVIDER_HOST = 1,
    MODEL_PROVIDER_LOCAL = 2
} model_provider_mode_t;

typedef enum {
    MODEL_STATUS_OFFLINE = 0,
    MODEL_STATUS_LOADING = 1,
    MODEL_STATUS_READY = 2,
    MODEL_STATUS_THINKING = 3,
    MODEL_STATUS_DRAWING_UI = 4
} model_status_t;

void model_provider_initialize(void);

void model_provider_set_mode(model_provider_mode_t mode);
model_provider_mode_t model_provider_get_mode(void);

void model_provider_set_status(model_status_t status);
model_status_t model_provider_get_status(void);

const char *model_provider_mode_name(void);
const char *model_provider_status_name(void);

void model_provider_load(const char *model_name);
void model_provider_ask(const char *prompt);

#endif
