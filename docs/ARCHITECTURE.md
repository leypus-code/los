# LOS Architecture

LOS is a small experimental 32-bit i386 operating system kernel.

Current stable version: LOS v20.12

## Boot

Files:

    kernel/boot.S
    linker.ld
    boot/grub.cfg

Responsibilities:

- Multiboot / GRUB boot
- initial kernel entry
- freestanding ELF kernel
- transfer control into C code

## CPU Layer

Files:

    kernel/cpu/

Responsibilities:

- GDT
- IDT
- ISR
- IRQ
- PIC
- assembly interrupt stubs

## Drivers

Files:

    kernel/drivers/

Current drivers:

- VGA text terminal
- keyboard
- PIT timer
- RTC

The terminal supports:

- colored text
- cursor movement
- screen save/restore
- muted output mode for quiet script execution

## Memory

Files:

    kernel/memory/

Current features:

- basic memory tracking
- physical page manager
- paging initialization

This is still an early-stage memory subsystem.

## Virtual Filesystem

Files:

    kernel/fs/vfs.c
    kernel/include/vfs.h

LOS currently uses a RAM-only VFS.

Supported concepts:

- directories
- files
- file content in memory
- path resolution
- create/remove/rename operations

Used by:

- shell
- editor
- Norton Commander UI
- script runner
- workspace system

## Shell

Files:

    kernel/shell/shell.c
    kernel/include/shell.h

Shell features:

- command execution
- command history
- cursor editing
- Tab completion
- file redirects
- help pager
- script runner
- workspace commands
- theme commands
- Linux-like utility commands

## Editor

Files:

    kernel/editor/editor.c
    kernel/include/editor.h

Current features:

- nano/edit commands
- open VFS files
- edit buffer
- save with F2
- exit with F10 or Esc
- vertical scrolling
- line/column footer
- themed UI

## UI Layer

Files:

    kernel/ui/
    kernel/ui_manager/
    kernel/uiblock/
    kernel/wm/
    kernel/layout/

Current UI components:

- pager
- Norton Commander-like file manager
- layout renderer
- UI blocks
- workspace rendering
- window manager stubs

## Theme Engine

Files:

    kernel/lib/theme.c
    kernel/include/theme.h

Used by:

- shell
- pager
- editor
- Norton Commander UI
- workspace UI
- UI blocks

Theme selector:

    themes

Direct theme command:

    theme pink
    theme matrix
    theme terminal

## Workspace System

Files:

    kernel/workspace/
    kernel/uiblock/
    kernel/layout/

The workspace system is the conceptual core of LOS.

A workspace is a structured generated screen made from:

- panels
- titles
- status blocks
- buttons
- layout nodes
- workspace files

## Experimental AI / Intent / Service Layers

Files:

    kernel/ai/
    kernel/intent/
    kernel/service/
    kernel/model/
    kernel/package/
    kernel/loader/

These are early stubs for the future AI-native direction of LOS.

The future idea:

- user describes an intent
- system builds a workspace
- workspace contains only needed controls/views/commands
- AI becomes a layer between user intent and low-level system functions
