#include "include/kernel.h"
#include "include/terminal.h"
#include "include/gdt.h"
#include "include/idt.h"
#include "include/pic.h"
#include "include/keyboard.h"
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

void kernel_panic(const char *message) {
    kprintf("\n\n[KERNEL PANIC] %s\nSystem halted.\n", message);

    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

void kernel_init(void) {
    terminal_initialize();
    terminal_enable_cursor();
    terminal_update_cursor();

    theme_print_banner();

    log_ok("Kernel entered");

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

    log_info("Enabling paging...");
    paging_enable_system();
    log_ok("Paging enabled");

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

    log_ok("CPU exceptions registered");
    log_ok("Hardware interrupts enabled");

    kprintf("\nWelcome to LOS.\n");
    kprintf("Kernel build: %s\n\n", "debug-i386");

    shell_initialize();

    __asm__ volatile ("sti");
}

void kernel_run(void) {
    while (1) {
        __asm__ volatile ("hlt");
    }
}
