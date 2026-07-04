#include "../include/paging.h"

#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x1
#define PAGE_WRITABLE 0x2

static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_table0[1024] __attribute__((aligned(4096)));
static uint32_t paging_enabled = 0;

extern void paging_load_directory(uint32_t directory);
extern void paging_enable(void);

void paging_initialize(void) {
    for (uint32_t i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
        page_table0[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    page_directory[0] = ((uint32_t)page_table0) | PAGE_PRESENT | PAGE_WRITABLE;
}

void paging_enable_system(void) {
    paging_load_directory((uint32_t)page_directory);
    paging_enable();
    paging_enabled = 1;
}

uint32_t paging_get_directory_addr(void) {
    return (uint32_t)page_directory;
}

uint32_t paging_get_table0_addr(void) {
    return (uint32_t)page_table0;
}

uint32_t paging_get_first_entry(void) {
    return page_table0[0];
}

uint32_t paging_is_enabled(void) {
    return paging_enabled;
}
