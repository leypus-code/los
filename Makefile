KERNEL=kernel.elf
GFX_KERNEL=kernel-gfx.elf
TEXT_KERNEL=kernel-text.elf
ISO=los.iso

CC=clang
LD=ld.lld

CFLAGS=-target i386-elf -ffreestanding -fno-stack-protector -fno-pic -Wall -Wextra
ASFLAGS=-target i386-elf
LDFLAGS=-m elf_i386 -T linker.ld -nostdlib

OBJS=kernel/boot.o \
     kernel/main.o \
     kernel/kernel.o \
     kernel/lib/string.o \
     kernel/lib/kprintf.o \
     kernel/lib/log.o \
     kernel/lib/theme.o \
     kernel/shell/shell.o \
     kernel/task/task.o \
     kernel/fs/vfs.o \
     kernel/ui/norton.o \
     kernel/ui/pager.o kernel/ui/gfx.o \
     kernel/app/app.o \
     kernel/ipc/ipc.o \
     kernel/log/eventlog.o \
     kernel/loader/loader.o \
     kernel/package/package.o \
     kernel/editor/editor.o \
     kernel/ui_manager/ui.o \
     kernel/wm/wm.o \
     kernel/ai/ai.o \
     kernel/ai/ring.o \
     kernel/ai/ai_bridge.o \
     kernel/model/model.o \
     kernel/service/service.o \
     kernel/intent/intent.o \
     kernel/layout/layout.o \
     kernel/uiblock/uiblock.o \
     kernel/workspace/workspace_builder.o \
     kernel/fileassoc/fileassoc.o \
     kernel/memory/memory.o \
     kernel/memory/pmm.o \
     kernel/memory/paging.o \
     kernel/memory/paging_enable.o \
     kernel/drivers/terminal.o \
     kernel/drivers/serial.o \
     kernel/drivers/keyboard.o \
     kernel/drivers/mouse.o \
     kernel/drivers/timer.o \
     kernel/drivers/rtc.o \
     kernel/cpu/gdt.o \
     kernel/cpu/gdt_flush.o \
     kernel/cpu/idt.o \
     kernel/cpu/idt_flush.o \
     kernel/cpu/isr.o \
     kernel/cpu/isr_asm.o \
     kernel/cpu/irq.o \
     kernel/cpu/irq_asm.o \
     kernel/cpu/pic.o

all: $(ISO)

kernel/boot.o: kernel/boot.S
	$(CC) $(ASFLAGS) -c kernel/boot.S -o kernel/boot.o

kernel/boot_gfx.o: kernel/boot_gfx.S
	$(CC) $(ASFLAGS) -c kernel/boot_gfx.S -o kernel/boot_gfx.o

kernel/boot_text.o: kernel/boot_text.S
	$(CC) $(ASFLAGS) -c kernel/boot_text.S -o kernel/boot_text.o

kernel/cpu/gdt_flush.o: kernel/cpu/gdt_flush.S
	$(CC) $(ASFLAGS) -c kernel/cpu/gdt_flush.S -o kernel/cpu/gdt_flush.o

kernel/cpu/idt_flush.o: kernel/cpu/idt_flush.S
	$(CC) $(ASFLAGS) -c kernel/cpu/idt_flush.S -o kernel/cpu/idt_flush.o

kernel/cpu/isr_asm.o: kernel/cpu/isr.S
	$(CC) $(ASFLAGS) -c kernel/cpu/isr.S -o kernel/cpu/isr_asm.o

kernel/cpu/irq_asm.o: kernel/cpu/irq.S
	$(CC) $(ASFLAGS) -c kernel/cpu/irq.S -o kernel/cpu/irq_asm.o

kernel/main.o: kernel/main.c
	$(CC) $(CFLAGS) -c kernel/main.c -o kernel/main.o

kernel/kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel/kernel.o

kernel/lib/string.o: kernel/lib/string.c
	$(CC) $(CFLAGS) -c kernel/lib/string.c -o kernel/lib/string.o

kernel/lib/kprintf.o: kernel/lib/kprintf.c
	$(CC) $(CFLAGS) -c kernel/lib/kprintf.c -o kernel/lib/kprintf.o

kernel/lib/log.o: kernel/lib/log.c
	$(CC) $(CFLAGS) -c kernel/lib/log.c -o kernel/lib/log.o

kernel/lib/theme.o: kernel/lib/theme.c
	$(CC) $(CFLAGS) -c kernel/lib/theme.c -o kernel/lib/theme.o

kernel/shell/shell.o: kernel/shell/shell.c
	$(CC) $(CFLAGS) -c kernel/shell/shell.c -o kernel/shell/shell.o

kernel/task/task.o: kernel/task/task.c
	$(CC) $(CFLAGS) -c kernel/task/task.c -o kernel/task/task.o

kernel/fs/vfs.o: kernel/fs/vfs.c
	$(CC) $(CFLAGS) -c kernel/fs/vfs.c -o kernel/fs/vfs.o

kernel/ui/norton.o: kernel/ui/norton.c
	$(CC) $(CFLAGS) -c kernel/ui/norton.c -o kernel/ui/norton.o

kernel/ui/pager.o: kernel/ui/pager.c
	$(CC) $(CFLAGS) -c kernel/ui/pager.c -o kernel/ui/pager.o

kernel/ui/gfx.o: kernel/ui/gfx.c
	$(CC) $(CFLAGS) -c kernel/ui/gfx.c -o kernel/ui/gfx.o

kernel/app/app.o: kernel/app/app.c
	$(CC) $(CFLAGS) -c kernel/app/app.c -o kernel/app/app.o

kernel/ipc/ipc.o: kernel/ipc/ipc.c
	$(CC) $(CFLAGS) -c kernel/ipc/ipc.c -o kernel/ipc/ipc.o

kernel/log/eventlog.o: kernel/log/eventlog.c
	$(CC) $(CFLAGS) -c kernel/log/eventlog.c -o kernel/log/eventlog.o

kernel/loader/loader.o: kernel/loader/loader.c
	$(CC) $(CFLAGS) -c kernel/loader/loader.c -o kernel/loader/loader.o

kernel/package/package.o: kernel/package/package.c
	$(CC) $(CFLAGS) -c kernel/package/package.c -o kernel/package/package.o

kernel/editor/editor.o: kernel/editor/editor.c
	$(CC) $(CFLAGS) -c kernel/editor/editor.c -o kernel/editor/editor.o

kernel/ui_manager/ui.o: kernel/ui_manager/ui.c
	$(CC) $(CFLAGS) -c kernel/ui_manager/ui.c -o kernel/ui_manager/ui.o

kernel/wm/wm.o: kernel/wm/wm.c
	$(CC) $(CFLAGS) -c kernel/wm/wm.c -o kernel/wm/wm.o

kernel/ai/ai.o: kernel/ai/ai.c
	$(CC) $(CFLAGS) -c kernel/ai/ai.c -o kernel/ai/ai.o

kernel/ai/ring.o: kernel/ai/ring.c
	$(CC) $(CFLAGS) -c kernel/ai/ring.c -o kernel/ai/ring.o

kernel/ai/ai_bridge.o: kernel/ai/ai_bridge.c
	$(CC) $(CFLAGS) -c kernel/ai/ai_bridge.c -o kernel/ai/ai_bridge.o

kernel/model/model.o: kernel/model/model.c
	$(CC) $(CFLAGS) -c kernel/model/model.c -o kernel/model/model.o

kernel/service/service.o: kernel/service/service.c
	$(CC) $(CFLAGS) -c kernel/service/service.c -o kernel/service/service.o

kernel/intent/intent.o: kernel/intent/intent.c
	$(CC) $(CFLAGS) -c kernel/intent/intent.c -o kernel/intent/intent.o

kernel/layout/layout.o: kernel/layout/layout.c
	$(CC) $(CFLAGS) -c kernel/layout/layout.c -o kernel/layout/layout.o

kernel/uiblock/uiblock.o: kernel/uiblock/uiblock.c
	$(CC) $(CFLAGS) -c kernel/uiblock/uiblock.c -o kernel/uiblock/uiblock.o

kernel/workspace/workspace_builder.o: kernel/workspace/workspace_builder.c
	$(CC) $(CFLAGS) -c kernel/workspace/workspace_builder.c -o kernel/workspace/workspace_builder.o

kernel/fileassoc/fileassoc.o: kernel/fileassoc/fileassoc.c
	$(CC) $(CFLAGS) -c kernel/fileassoc/fileassoc.c -o kernel/fileassoc/fileassoc.o

kernel/memory/memory.o: kernel/memory/memory.c
	$(CC) $(CFLAGS) -c kernel/memory/memory.c -o kernel/memory/memory.o

kernel/memory/pmm.o: kernel/memory/pmm.c
	$(CC) $(CFLAGS) -c kernel/memory/pmm.c -o kernel/memory/pmm.o

kernel/memory/paging.o: kernel/memory/paging.c
	$(CC) $(CFLAGS) -c kernel/memory/paging.c -o kernel/memory/paging.o

kernel/memory/paging_enable.o: kernel/memory/paging_enable.S
	$(CC) $(ASFLAGS) -c kernel/memory/paging_enable.S -o kernel/memory/paging_enable.o

kernel/drivers/serial.o: kernel/drivers/serial.c
	$(CC) $(CFLAGS) -c kernel/drivers/serial.c -o kernel/drivers/serial.o

kernel/drivers/terminal.o: kernel/drivers/terminal.c
	$(CC) $(CFLAGS) -c kernel/drivers/terminal.c -o kernel/drivers/terminal.o

kernel/drivers/keyboard.o: kernel/drivers/keyboard.c
	$(CC) $(CFLAGS) -c kernel/drivers/keyboard.c -o kernel/drivers/keyboard.o

kernel/drivers/mouse.o: kernel/drivers/mouse.c
	$(CC) $(CFLAGS) -c kernel/drivers/mouse.c -o kernel/drivers/mouse.o

kernel/drivers/timer.o: kernel/drivers/timer.c
	$(CC) $(CFLAGS) -c kernel/drivers/timer.c -o kernel/drivers/timer.o

kernel/drivers/rtc.o: kernel/drivers/rtc.c
	$(CC) $(CFLAGS) -c kernel/drivers/rtc.c -o kernel/drivers/rtc.o

kernel/cpu/gdt.o: kernel/cpu/gdt.c
	$(CC) $(CFLAGS) -c kernel/cpu/gdt.c -o kernel/cpu/gdt.o

kernel/cpu/idt.o: kernel/cpu/idt.c
	$(CC) $(CFLAGS) -c kernel/cpu/idt.c -o kernel/cpu/idt.o

kernel/cpu/isr.o: kernel/cpu/isr.c
	$(CC) $(CFLAGS) -c kernel/cpu/isr.c -o kernel/cpu/isr.o

kernel/cpu/irq.o: kernel/cpu/irq.c
	$(CC) $(CFLAGS) -c kernel/cpu/irq.c -o kernel/cpu/irq.o

kernel/cpu/pic.o: kernel/cpu/pic.c
	$(CC) $(CFLAGS) -c kernel/cpu/pic.c -o kernel/cpu/pic.o

BASE_OBJS=$(filter-out kernel/boot.o,$(OBJS))
GFX_OBJS=kernel/boot_gfx.o $(BASE_OBJS)
TEXT_OBJS=kernel/boot_text.o $(BASE_OBJS)

$(KERNEL): $(GFX_KERNEL)
	cp $(GFX_KERNEL) $(KERNEL)

$(GFX_KERNEL): $(GFX_OBJS) linker.ld
	$(LD) $(LDFLAGS) $(GFX_OBJS) -o $(GFX_KERNEL)

$(TEXT_KERNEL): $(TEXT_OBJS) linker.ld
	$(LD) $(LDFLAGS) $(TEXT_OBJS) -o $(TEXT_KERNEL)

$(ISO): $(GFX_KERNEL) $(TEXT_KERNEL)
	mkdir -p iso_root/boot/grub
	cp $(GFX_KERNEL) iso_root/boot/kernel-gfx.elf
	cp $(TEXT_KERNEL) iso_root/boot/kernel-text.elf
	cp boot/grub.cfg iso_root/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso_root

run: $(ISO)
	qemu-system-i386 -machine pc -vga std -cdrom $(ISO) -no-shutdown


run-control: $(ISO)
	qemu-system-i386 -machine pc -vga std -cdrom $(ISO) -no-shutdown

run-ai: $(ISO)
	qemu-system-i386 -machine pc -vga std -cdrom $(ISO) -no-shutdown -serial tcp:127.0.0.1:7777,server,nowait

clean:
	rm -rf kernel/*.o kernel/drivers/*.o kernel/cpu/*.o kernel/lib/*.o kernel/shell/*.o kernel/task/*.o kernel/fs/*.o kernel/ui/*.o kernel/app/*.o kernel/ipc/*.o kernel/log/*.o kernel/loader/*.o kernel/package/*.o kernel/editor/*.o kernel/ui_manager/*.o kernel/wm/*.o kernel/ai/*.o kernel/model/*.o kernel/service/*.o kernel/intent/*.o kernel/layout/*.o kernel/uiblock/*.o kernel/workspace/*.o kernel/fileassoc/*.o kernel/memory/*.o $(KERNEL) $(GFX_KERNEL) $(TEXT_KERNEL) $(ISO) iso_root/boot
