#ifndef LOS_TIMER_H
#define LOS_TIMER_H

#include <stdint.h>

void timer_initialize(uint32_t frequency);
uint32_t timer_get_ticks(void);

#endif
