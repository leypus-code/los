# LOS — Lightweight Operating System

LOS is a small experimental 32-bit x86 operating system kernel.

It is built as a freestanding Multiboot2 kernel and currently runs in QEMU from a GRUB ISO image.

Current stable version: LOS v20.12

## Goal

LOS is an experimental AI-native / workspace-native operating system concept.

The long-term idea is not just to clone Unix or Windows, but to build an operating system where the main abstraction is a generated workspace: a dynamic screen made of terminal blocks, file views, editor panels, logs, commands, buttons, scripts, and context-specific tools.

The current project is still a hobby kernel prototype, but it already contains a working foundation:

- bootable i386 kernel
- VGA terminal
- keyboard input
- shell
- RAM virtual filesystem
- editor
- themes
- scripts
- workspace system
- UI blocks
- service / intent / AI stubs

## Build and Run

Required tools:

- clang
- lld
- grub-mkrescue
- xorriso
- qemu-system-i386
- make

Build and run:

    make clean
    make run

This creates:

    kernel.elf
    los.iso

and boots LOS in QEMU.

## Current Features

### Kernel

- 32-bit i386 freestanding kernel
- Multiboot / GRUB boot
- GDT / IDT
- ISR / IRQ
- PIC
- PIT timer
- RTC
- keyboard driver
- VGA text terminal
- basic memory manager
- paging support

### Terminal and Shell

- interactive shell
- command history with Up / Down
- cursor movement with Left / Right
- editing inside current command line
- Backspace support
- Tab completion for commands, paths, and themes
- command trimming before execution
- terminal scrollback commands

### Virtual Filesystem

LOS currently uses a RAM-only VFS.

Supported operations:

- directories
- files
- path resolution
- create files
- create directories
- remove files
- rename files
- read/write file contents

Common commands:

    ls
    tree
    cd path
    cat file
    mkdir name
    mkdir -p path
    touch name
    rm name
    rm -r path
    rename old new
    cp source target
    mv source target

Redirects:

    echo text > file
    echo text >> file

### Editor

LOS has a small nano-like editor.

Commands:

    nano file
    edit file

Controls:

    F2      Save
    F10     Exit
    Esc     Exit
    Arrows  Move cursor
    PgUp    Scroll up
    PgDn    Scroll down

Current editor features:

- opens VFS files
- edits text buffer
- saves file
- 4096-byte buffer
- vertical scrolling
- line/column footer
- themed UI

### Themes

Theme commands:

    themes
    theme
    theme list
    theme next
    theme prev
    theme name

Examples:

    theme terminal
    theme matrix
    theme pink
    theme amber
    theme ice

The theme engine is used by:

- shell
- pager
- editor
- Norton Commander-like UI
- workspace UI blocks

### Scripts

LOS supports .los scripts.

Quiet mode:

    run file.los

Run startup script manually:

    startup

Verbose mode:

    run -v file.los

Example:

    echo theme pink > /scripts/pink.los
    echo ls >> /scripts/pink.los
    run /scripts/pink.los
    run -v /scripts/pink.los

### UI

Current UI commands:

    nc
    wm
    currentapp

`nc` opens a Norton Commander-like file manager.

### Workspaces

Workspace commands:

    workspaces
    open file.workspace
    mkworkspace file.workspace title
    workspace file.workspace
    workstatus file.workspace status
    wstitle file.workspace title
    wsadd file.workspace type title content
    wsbutton file.workspace title label action
    wsnode file.workspace root|row|column vertical|horizontal [weight]
    wsend file.workspace

The workspace system is the conceptual core of LOS.

A workspace is a structured generated screen made from UI blocks, panels, buttons, titles, status sections, and layout nodes.

### Experimental Layers

LOS already contains early stubs for future high-level systems:

- app registry
- IPC
- event log
- service layer
- intent layer
- model layer
- package layer
- loader layer
- AI command layer

Commands include:

    ai
    aistatus
    services
    service
    apps
    runapp
    handlers
    models
    modelstatus
    importmodel
    loadmodel
    packages
    install
    remove
    formats
    load

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

## Version

Version is stored in:

    kernel/include/version.h

Changelog:

    CHANGELOG_LOS.md

## Current Stable Point

Current stable point:

    LOS v20.12

Important features at this point:

- clean build
- QEMU boot
- themed shell/UI/editor/workspaces
- shell history
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

    wstemplate coding /workspaces/coding.workspace
    open /workspaces/coding.workspace

    wstemplate system /workspaces/system.workspace
    open /workspaces/system.workspace

    wstemplate notes /workspaces/notes.workspace
    open /workspaces/notes.workspace

    wstemplate services /workspaces/services.workspace
    open /workspaces/services.workspace

Available templates:

    coding
    system
    notes
    services


## Workspace button actions

Workspace button blocks can execute service actions or shell commands.

Shell action format:

    shell:<command>

Examples:

    shell:run /scripts/build.los
    shell:echo TODO > /notes/todo.txt
    shell:theme matrix
    shell:nc

Direct supported action prefixes:

    run ...
    open ...
    nc
    nano ...
    edit ...
    theme ...

Example workspace block line:

    BLOCK=button|Run Build|Build|shell:run /scripts/build.los


## Intent-driven workspaces

LOS can create and open workspaces from simple rule-based intents.

Examples:

    intent "create coding workspace"
    intent "open coding workspace"
    intent "create and open coding workspace"

    intent "create system workspace"
    intent "open system workspace"

    intent "create notes workspace"
    intent "open notes workspace"

    intent "create services workspace"
    intent "open services workspace"

The ai command also tries intent handling first:

    ai "create and open coding workspace"


## Generated task workspaces

LOS can create task-specific workspace screens from intent phrases.

Examples:

    intent "debug build error"
    intent "system overview"
    intent "write notes"
    intent "plan project"

Shortcut command:

    gentask debug
    gentask overview
    gentask writing
    gentask planning

These commands generate workspace files under /workspaces and open them as task screens.


## Task files

Generated task workspaces create matching task metadata files under /workspaces.

Example:

    intent "debug build error"

Creates:

    /workspaces/debug-build.workspace
    /workspaces/debug-build.task

List tasks:

    tasks

Inspect a task file:

    cat /workspaces/debug-build.task

Task files contain:

    TASK
    TITLE=...
    INTENT=...
    KIND=...
    WORKSPACE=...
    STATUS=open
    NEXT=...


## Task commands

Task files can be listed, inspected, opened, and marked done.

Examples:

    tasklist
    taskshow debug-build
    taskopen debug-build
    taskdone debug-build

Task names may be given as:

    debug-build
    debug-build.task
    /workspaces/debug-build.task
