#ifndef LOS_MEMORY_H
#define LOS_MEMORY_H

#include <stddef.h>
#include <stdint.h>

void memory_initialize(void);
void *kmalloc(size_t size);
void kfree(void *ptr);

uint32_t memory_get_heap_start(void);
uint32_t memory_get_heap_current(void);

#endif
