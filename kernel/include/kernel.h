#ifndef LOS_KERNEL_H
#define LOS_KERNEL_H

#include <stdint.h>

void kernel_set_boot_info(uint32_t magic, uint32_t mbi_addr);
uint32_t kernel_get_boot_magic(void);
uint32_t kernel_get_boot_mbi_addr(void);
void kernel_init(void);
void kernel_run(void);
void kernel_panic(const char *message);

#endif
