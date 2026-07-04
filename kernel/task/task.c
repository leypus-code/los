#include "../include/task.h"
#include "../include/memory.h"

static task_t *task_list = 0;
static task_t *task_tail = 0;
static task_t *current_task = 0;
static uint32_t next_pid = 1;
static uint32_t task_count = 0;

static void copy_name(char *dst, const char *src) {
    uint32_t i = 0;

    while (src[i] && i < 31) {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
}

void task_initialize(void) {
    task_list = 0;
    task_tail = 0;
    current_task = 0;
    next_pid = 1;
    task_count = 0;

    task_t *kernel = task_create("kernel");
    kernel->state = TASK_RUNNING;
    current_task = kernel;
}

task_t *task_create(const char *name) {
    task_t *task = (task_t *)kmalloc(sizeof(task_t));

    if (!task) {
        return 0;
    }

    task->pid = next_pid++;
    copy_name(task->name, name);
    task->state = TASK_READY;
    task->next = 0;

    if (!task_list) {
        task_list = task;
        task_tail = task;
    } else {
        task_tail->next = task;
        task_tail = task;
    }

    task_count++;

    return task;
}

task_t *task_get_list(void) {
    return task_list;
}

task_t *task_get_current(void) {
    return current_task;
}

uint32_t task_get_count(void) {
    return task_count;
}

void scheduler_tick(void) {
    if (!current_task || !task_list) {
        return;
    }

    current_task->state = TASK_READY;

    if (current_task->next) {
        current_task = current_task->next;
    } else {
        current_task = task_list;
    }

    current_task->state = TASK_RUNNING;
}

void task_set_state(task_t *task, task_state_t state) {
    if (!task) {
        return;
    }

    task->state = state;
}
