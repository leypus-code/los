# LOS Development

Current stable version: LOS v20.12

## Build

Build and run:

    make clean
    make run

This creates:

    kernel.elf
    los.iso

and boots LOS in QEMU.

## Required Tools

    clang
    ld.lld
    grub-mkrescue
    xorriso
    qemu-system-i386
    make

## Ignored Build Artifacts

The following files are ignored by git:

    *.o
    *.elf
    *.iso
    iso_root/

## Version

Version is stored in:

    kernel/include/version.h

Example:

    #define LOS_VERSION "LOS v20.12"
    #define LOS_VERSION_SHORT "v20.12"

Changelog:

    CHANGELOG_LOS.md

## Project Structure

    kernel/cpu/          CPU, GDT, IDT, ISR, IRQ, PIC
    kernel/drivers/      terminal, keyboard, timer, RTC
    kernel/fs/           virtual filesystem
    kernel/shell/        shell and commands
    kernel/ui/           pager and Norton UI
    kernel/editor/       built-in editor
    kernel/lib/          string, printf, log, theme
    kernel/memory/       memory and paging
    kernel/workspace/    workspace builder
    kernel/uiblock/      workspace UI blocks
    kernel/layout/       layout system
    kernel/intent/       intent layer stubs
    kernel/service/      service layer stubs
    kernel/ai/           AI layer stubs
    kernel/model/        model layer stubs
    kernel/package/      package layer stubs
    kernel/loader/       loader layer stubs

## Development Rule

Keep each step small.

Recommended flow:

    make clean
    make run

Then test the changed feature inside QEMU.

## Current Stable Point

    LOS v20.12

Startup script:

    /scripts/startup.los

Important features at this point:

- clean build
- QEMU boot
- themed shell/UI/editor/workspaces
- command history
- cursor editing
- Tab completion
- echo redirects
- quiet/verbose script runner
- nano-like editor with vertical scroll
- workspace builder commands


## Quoted arguments

LOS shell supports quoted arguments for selected commands.

Examples:

    mkdir -p "/projects/my app"
    echo hello > "/notes/hello.txt"
    cp "/notes/hello.txt" "/notes/hello copy.txt"
    mv "/notes/hello copy.txt" "/notes/final note.txt"
    wstitle main.workspace "Main Workspace"
    wsadd main.workspace panel "System Status" "Everything is OK"
    wsbutton main.workspace "Actions" "Build" "run /scripts/build.los"


## Workspace templates

LOS can generate workspace files from predefined templates.

Examples:

    wstemplate coding coding.workspace
    open coding.workspace

    wstemplate system system.workspace
    open system.workspace

    wstemplate notes notes.workspace
    open notes.workspace

    wstemplate services services.workspace
    open services.workspace

Available templates:

    coding
    system
    notes
    services
