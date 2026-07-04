#ifndef LOS_SHELL_H
#define LOS_SHELL_H

void shell_initialize(void);
void shell_putchar(char c);
void shell_handle_key(int key);
void shell_resume_from_ui(void);
void shell_show_prompt(void);

int shell_is_ui_mode(void);
int shell_get_ui_mode(void);
void shell_set_ui_mode(int mode);

#endif
