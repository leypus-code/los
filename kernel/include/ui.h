#ifndef LOS_UI_H
#define LOS_UI_H

#define UI_NONE   0
#define UI_NORTON 1
#define UI_EDITOR 2
#define UI_PAGER  3
#define UI_WM     4
#define UI_LAYOUT 5

void ui_initialize(void);
void ui_enter(int mode);
void ui_push(int mode);
int ui_pop(void);
void ui_exit_to_shell(void);
int ui_current(void);
int ui_previous(void);
const char *ui_name(int mode);

#endif
