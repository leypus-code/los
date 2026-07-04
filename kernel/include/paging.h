#ifndef LOS_PAGING_H
#define LOS_PAGING_H

#include <stdint.h>

void paging_initialize(void);
void paging_enable_system(void);

uint32_t paging_get_directory_addr(void);
uint32_t paging_get_table0_addr(void);
uint32_t paging_get_first_entry(void);
uint32_t paging_is_enabled(void);

#endif
