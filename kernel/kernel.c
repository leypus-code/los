#include "include/kernel.h"
#include "include/terminal.h"
#include "include/serial.h"
#include "include/gdt.h"
#include "include/idt.h"
#include "include/pic.h"
#include "include/keyboard.h"
#include "include/mouse.h"
#include "include/shell.h"
#include "include/kprintf.h"
#include "include/log.h"
#include "include/theme.h"
#include "include/timer.h"
#include "include/memory.h"
#include "include/task.h"
#include "include/vfs.h"
#include "include/app.h"
#include "include/ipc.h"
#include "include/eventlog.h"
#include "include/loader.h"
#include "include/package.h"
#include "include/ui.h"
#include "include/wm.h"
#include "include/ai.h"
#include "include/model.h"
#include "include/service.h"
#include "include/intent.h"
#include "include/ring.h"
#include "include/layout.h"
#include "include/workspace_builder.h"
#include "include/fileassoc.h"
#include "include/pmm.h"
#include "include/paging.h"
#include "include/gfx.h"
#include "include/multiboot2.h"
#include "include/io.h"


static uint32_t kernel_boot_magic = 0;
static uint32_t kernel_boot_mbi_addr = 0;

void kernel_set_boot_info(uint32_t magic, uint32_t mbi_addr) {
    kernel_boot_magic = magic;
    kernel_boot_mbi_addr = mbi_addr;
}

uint32_t kernel_get_boot_magic(void) {
    return kernel_boot_magic;
}

uint32_t kernel_get_boot_mbi_addr(void) {
    return kernel_boot_mbi_addr;
}


static int kernel_fb_found = 0;
static uint32_t kernel_fb_addr_low = 0;
static uint32_t kernel_fb_addr_high = 0;
static uint32_t kernel_fb_pitch = 0;
static uint32_t kernel_fb_width = 0;
static uint32_t kernel_fb_height = 0;
static uint32_t kernel_fb_bpp = 0;
static uint32_t kernel_fb_type = 0;

static uint32_t kernel_align8(uint32_t value) {
    return (value + 7) & ~7;
}

void kernel_multiboot_parse(void) {
    uint32_t total_size = 0;
    uint32_t ptr = 0;
    uint32_t end = 0;

    kernel_fb_found = 0;

    if (kernel_boot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        return;
    }

    if (!kernel_boot_mbi_addr) {
        return;
    }

    total_size = *((uint32_t *)kernel_boot_mbi_addr);
    ptr = kernel_boot_mbi_addr + 8;
    end = kernel_boot_mbi_addr + total_size;

    while (ptr + sizeof(multiboot_tag_t) <= end) {
        multiboot_tag_t *tag = (multiboot_tag_t *)ptr;

        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            break;
        }

        if (tag->size < sizeof(multiboot_tag_t)) {
            break;
        }

        if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            multiboot_tag_framebuffer_t *fb = (multiboot_tag_framebuffer_t *)ptr;

            kernel_fb_addr_low = (uint32_t)(fb->framebuffer_addr & 0xffffffff);
            kernel_fb_addr_high = (uint32_t)(fb->framebuffer_addr >> 32);
            kernel_fb_pitch = fb->framebuffer_pitch;
            kernel_fb_width = fb->framebuffer_width;
            kernel_fb_height = fb->framebuffer_height;
            kernel_fb_bpp = fb->framebuffer_bpp;
            kernel_fb_type = fb->framebuffer_type;
            kernel_fb_found = 1;
        }

        ptr += kernel_align8(tag->size);
    }
}

int kernel_framebuffer_available(void) {
    return kernel_fb_found;
}

uint32_t kernel_framebuffer_addr_low(void) {
    return kernel_fb_addr_low;
}

uint32_t kernel_framebuffer_addr_high(void) {
    return kernel_fb_addr_high;
}

uint32_t kernel_framebuffer_pitch(void) {
    return kernel_fb_pitch;
}

uint32_t kernel_framebuffer_width(void) {
    return kernel_fb_width;
}

uint32_t kernel_framebuffer_height(void) {
    return kernel_fb_height;
}

uint32_t kernel_framebuffer_bpp(void) {
    return kernel_fb_bpp;
}

uint32_t kernel_framebuffer_type(void) {
    return kernel_fb_type;
}

void kernel_reboot(void) {
    /*
     * Hardware reset via PS/2 controller.
     * Command 0xFE pulses CPU reset line.
     * In QEMU this returns to BIOS/GRUB, so the boot menu appears again.
     */
    __asm__ volatile ("cli");

    for (uint32_t i = 0; i < 100000; i++) {
        if ((inb(0x64) & 0x02) == 0) {
            break;
        }
    }

    outb(0x64, 0xFE);

    /*
     * Fallback: if reset command does not fire, halt forever.
     */
    while (1) {
        __asm__ volatile ("hlt");
    }
}

void kernel_panic(const char *message) {
    kprintf("\n\n[KERNEL PANIC] %s\nSystem halted.\n", message);

    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

void kernel_init(void) {
    terminal_initialize();
    serial_initialize();
    terminal_enable_cursor();
    terminal_update_cursor();

    theme_print_banner();

    log_ok("Kernel entered");

    log_info("Parsing Multiboot2 information...");
    kernel_multiboot_parse();
    if (kernel_framebuffer_available()) {
        log_ok("Framebuffer information found");
    } else {
        log_info("Framebuffer information not available yet");
    }

    gfx_initialize_from_kernel();

    log_info("Initializing GDT...");
    gdt_initialize();
    log_ok("GDT initialized");

    log_info("Initializing IDT...");
    idt_initialize();
    log_ok("IDT initialized");

    log_info("Remapping PIC...");
    pic_remap();
    log_ok("PIC remapped");

    log_info("Initializing memory...");
    memory_initialize();
    log_ok("Heap initialized");

    log_info("Initializing physical memory manager...");
    pmm_initialize();
    log_ok("Physical memory manager initialized");

    log_info("Initializing paging structures...");
    paging_initialize();
    log_ok("Paging structures initialized");

    if (kernel_framebuffer_available()) {
        /*
         * Framebuffer is a physical address provided by Multiboot2.
         * Until framebuffer memory is explicitly mapped into page tables,
         * enabling paging makes gfx_fb unsafe after boot.
         *
         * Keep paging disabled in framebuffer UI mode so keyboard-driven
         * redraws of the AI surface/input bar work reliably.
         */
        log_info("Paging skipped for framebuffer UI mode");
    } else {
        log_info("Enabling paging...");
        paging_enable_system();
        log_ok("Paging enabled");
    }

    log_info("Initializing task subsystem...");
    task_initialize();
    log_ok("Task subsystem initialized");

    log_info("Initializing virtual file system...");
    vfs_initialize();
    log_ok("Virtual file system initialized");

    log_info("Initializing event log...");
    eventlog_initialize();
    log_ok("Event log initialized");

    log_info("Initializing application manager...");
    app_initialize();

    log_info("Initializing IPC message queue...");
    ipc_initialize();
    log_ok("IPC initialized");

    log_info("Initializing LAP loader...");
    loader_initialize();
    log_ok("LAP loader initialized");

    log_info("Initializing package manager...");
    package_initialize();
    log_ok("Package manager initialized");

    log_info("Initializing UI manager...");
    ui_initialize();
    log_ok("UI manager initialized");

    log_info("Initializing window manager...");
    wm_initialize();
    log_ok("Window manager initialized");

    log_info("Initializing model manager...");
    model_initialize();
    log_ok("Model manager initialized");

    log_info("Initializing AI runtime...");
    ai_initialize();
    log_ok("AI runtime initialized");

    log_info("Initializing service bus...");
    service_initialize();
    log_ok("Service bus initialized");

    log_info("Initializing intent engine...");
    intent_initialize();
    ring_initialize();
    log_ok("Intent engine initialized");

    log_info("Initializing layout engine...");
    layout_initialize();
    log_ok("Layout engine initialized");

    log_info("Initializing file associations...");
    fileassoc_initialize();
    log_ok("File associations initialized");

    log_info("Initializing workspace builder...");
    workspace_builder_initialize();
    log_ok("Workspace builder initialized");

    log_info("Initializing timer...");
    timer_initialize(100);
    log_ok("Timer initialized");

    log_info("Initializing keyboard...");
    keyboard_initialize();
    log_ok("Keyboard initialized");

    log_info("Initializing mouse...");
    mouse_initialize();
    log_ok("Mouse initialized");

    log_ok("CPU exceptions registered");
    log_ok("Hardware interrupts enabled");

    kprintf("\nWelcome to LOS.\n");
    kprintf("Kernel build: %s\n\n", "debug-i386");

    shell_initialize();

    if (gfx_is_ready()) {
        gfx_draw_ai_surface();
        shell_set_ui_mode(0);
    }

    __asm__ volatile ("sti");
}

void kernel_run(void) {
    while (1) {
        keyboard_poll();
        mouse_poll();

        if (gfx_is_ready()) {
            gfx_tick();
        }

        __asm__ volatile ("pause");
    }
}










