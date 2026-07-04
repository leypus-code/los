#include "include/kernel.h"
#include "include/multiboot2.h"

void kernel_main(uint32_t magic, uint32_t mbi_addr) {
    kernel_set_boot_info(magic, mbi_addr);
    kernel_init();
    kernel_run();
}
