#ifndef LOS_TASK_H
#define LOS_TASK_H

#include <stdint.h>

typedef enum {
    TASK_READY = 0,
    TASK_RUNNING = 1,
    TASK_SLEEPING = 2,
    TASK_DEAD = 3
} task_state_t;

typedef struct task {
    uint32_t pid;
    char name[32];
    task_state_t state;
    struct task *next;
} task_t;

void task_initialize(void);
task_t *task_create(const char *name);
void task_set_state(task_t *task, task_state_t state);
task_t *task_get_list(void);
task_t *task_get_current(void);
uint32_t task_get_count(void);
void scheduler_tick(void);

#endif
