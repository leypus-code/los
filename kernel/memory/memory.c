#include "../include/memory.h"

#define HEAP_SIZE 0x100000

typedef struct heap_block {
    uint32_t size;
    uint32_t free;
    struct heap_block *next;
} heap_block_t;

extern uint32_t kernel_end;

static uint32_t heap_start = 0;
static uint32_t heap_current = 0;
static heap_block_t *heap_head = 0;

static uint32_t align16(uint32_t value) {
    return (value + 15) & ~15;
}

void memory_initialize(void) {
    heap_start = align16((uint32_t)&kernel_end);
    heap_current = heap_start;

    heap_head = (heap_block_t *)heap_start;
    heap_head->size = HEAP_SIZE - sizeof(heap_block_t);
    heap_head->free = 1;
    heap_head->next = 0;
}

void *kmalloc(size_t size) {
    if (size == 0) {
        return 0;
    }

    size = align16(size);

    heap_block_t *current = heap_head;

    while (current) {
        if (current->free && current->size >= size) {
            if (current->size > size + sizeof(heap_block_t) + 16) {
                heap_block_t *new_block =
                    (heap_block_t *)((uint32_t)current + sizeof(heap_block_t) + size);

                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->free = 1;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }

            current->free = 0;
            heap_current = (uint32_t)current + sizeof(heap_block_t) + current->size;

            return (void *)((uint32_t)current + sizeof(heap_block_t));
        }

        current = current->next;
    }

    return 0;
}

void kfree(void *ptr) {
    if (!ptr) {
        return;
    }

    heap_block_t *block = (heap_block_t *)((uint32_t)ptr - sizeof(heap_block_t));
    block->free = 1;

    heap_block_t *current = heap_head;

    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += sizeof(heap_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

uint32_t memory_get_heap_start(void) {
    return heap_start;
}

uint32_t memory_get_heap_current(void) {
    return heap_current;
}
