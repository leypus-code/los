#ifndef LOS_KEYBOARD_H
#define LOS_KEYBOARD_H

#include <stdint.h>

void keyboard_initialize(void);
void keyboard_poll(void);
int keyboard_is_capslock_on(void);
int keyboard_is_shift_down(void);
uint32_t keyboard_get_shift_presses(void);

#endif
