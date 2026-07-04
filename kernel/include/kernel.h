#ifndef LOS_KERNEL_H
#define LOS_KERNEL_H

#include <stdint.h>

void kernel_set_boot_info(uint32_t magic, uint32_t mbi_addr);
uint32_t kernel_get_boot_magic(void);
uint32_t kernel_get_boot_mbi_addr(void);
void kernel_multiboot_parse(void);
int kernel_framebuffer_available(void);
uint32_t kernel_framebuffer_addr_low(void);
uint32_t kernel_framebuffer_addr_high(void);
uint32_t kernel_framebuffer_pitch(void);
uint32_t kernel_framebuffer_width(void);
uint32_t kernel_framebuffer_height(void);
uint32_t kernel_framebuffer_bpp(void);
uint32_t kernel_framebuffer_type(void);
void kernel_init(void);
void kernel_run(void);
void kernel_panic(const char *message);

#endif
