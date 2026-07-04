#ifndef LOS_PMM_H
#define LOS_PMM_H

#include <stdint.h>

void pmm_initialize(void);
void *pmm_alloc_page(void);
void pmm_free_page(void *page);

uint32_t pmm_get_total_pages(void);
uint32_t pmm_get_used_pages(void);
uint32_t pmm_get_free_pages(void);

#endif
