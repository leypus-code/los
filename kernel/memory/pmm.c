#include "../include/pmm.h"
#include "../include/memory.h"

#define PAGE_SIZE 4096
#define PMM_START 0x00200000
#define PMM_END   0x01000000

#define PMM_TOTAL_PAGES ((PMM_END - PMM_START) / PAGE_SIZE)
#define BITMAP_SIZE     (PMM_TOTAL_PAGES / 8)

static uint8_t bitmap[BITMAP_SIZE];
static uint32_t total_pages = PMM_TOTAL_PAGES;
static uint32_t used_pages = 0;

static void bitmap_set(uint32_t bit) {
    bitmap[bit / 8] |= (1 << (bit % 8));
}

static void bitmap_clear(uint32_t bit) {
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static uint8_t bitmap_test(uint32_t bit) {
    return bitmap[bit / 8] & (1 << (bit % 8));
}

void pmm_initialize(void) {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        bitmap[i] = 0;
    }

    used_pages = 0;
}

void *pmm_alloc_page(void) {
    for (uint32_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_pages++;

            return (void *)(PMM_START + i * PAGE_SIZE);
        }
    }

    return 0;
}

void pmm_free_page(void *page) {
    uint32_t addr = (uint32_t)page;

    if (addr < PMM_START || addr >= PMM_END) {
        return;
    }

    if (addr % PAGE_SIZE != 0) {
        return;
    }

    uint32_t index = (addr - PMM_START) / PAGE_SIZE;

    if (bitmap_test(index)) {
        bitmap_clear(index);
        used_pages--;
    }
}

uint32_t pmm_get_total_pages(void) {
    return total_pages;
}

uint32_t pmm_get_used_pages(void) {
    return used_pages;
}

uint32_t pmm_get_free_pages(void) {
    return total_pages - used_pages;
}
