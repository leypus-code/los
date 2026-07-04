#ifndef LOS_THEME_H
#define LOS_THEME_H

#include "terminal.h"

void theme_print_banner(void);
void theme_log_ok(const char *message);
void theme_log_info(const char *message);
void theme_log_error(const char *message);
void theme_prompt(void);

void theme_set_normal(void);
void theme_set_title(void);
void theme_set_dir_tag(void);
void theme_set_dir_name(void);
void theme_set_file_tag(void);
void theme_set_root(void);
void theme_set_tree_line(void);
void theme_set_error(void);

void theme_list(void);
int theme_set_scheme(const char *name);
const char *theme_current_name(void);
void theme_clear_screen(void);
void theme_repaint_screen(void);
void theme_next(void);
void theme_prev(void);

int theme_count(void);
int theme_current_index(void);
const char *theme_name_at(int index);
const char *theme_label_at(int index);
int theme_set_index(int index);

/* Raw colors for apps/UI systems */
uint8_t theme_color_normal(void);
uint8_t theme_color_title(void);
uint8_t theme_color_footer(void);
uint8_t theme_color_border(void);
uint8_t theme_color_selected(void);
uint8_t theme_color_dialog(void);
uint8_t theme_color_dialog_text(void);
uint8_t theme_color_input(void);
uint8_t theme_color_error(void);
uint8_t theme_color_ok(void);
uint8_t theme_color_info(void);
uint8_t theme_color_panel(void);

#endif
