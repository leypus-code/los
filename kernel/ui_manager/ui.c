#include "../include/ui.h"
#include "../include/shell.h"

#define UI_STACK_MAX 8

static int current_ui = UI_NONE;
static int stack[UI_STACK_MAX];
static int stack_count = 0;

void ui_initialize(void) {
    current_ui = UI_NONE;
    stack_count = 0;
}

void ui_enter(int mode) {
    current_ui = mode;
    shell_set_ui_mode(mode);
}

void ui_push(int mode) {
    if (stack_count < UI_STACK_MAX) {
        stack[stack_count++] = current_ui;
    }

    current_ui = mode;
    shell_set_ui_mode(mode);
}

int ui_pop(void) {
    if (stack_count <= 0) {
        current_ui = UI_NONE;
        shell_set_ui_mode(UI_NONE);
        return UI_NONE;
    }

    current_ui = stack[--stack_count];
    shell_set_ui_mode(current_ui);
    return current_ui;
}

void ui_exit_to_shell(void) {
    stack_count = 0;
    current_ui = UI_NONE;
    shell_set_ui_mode(UI_NONE);
    shell_resume_from_ui();
}

int ui_current(void) {
    return current_ui;
}

int ui_previous(void) {
    if (stack_count <= 0) return UI_NONE;
    return stack[stack_count - 1];
}

const char *ui_name(int mode) {
    if (mode == UI_NORTON) return "norton";
    if (mode == UI_EDITOR) return "editor";
    if (mode == UI_PAGER) return "pager";
    return "shell";
}
