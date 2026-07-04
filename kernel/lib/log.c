#include "../include/log.h"
#include "../include/theme.h"

void log_info(const char *message) {
    theme_log_info(message);
}

void log_ok(const char *message) {
    theme_log_ok(message);
}

void log_error(const char *message) {
    theme_log_error(message);
}
