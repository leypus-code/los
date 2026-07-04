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


## Task lifecycle commands

Task files support status changes and next actions.

Examples:

    tasklist
    taskshow debug-build
    taskstatus debug-build active
    tasknext debug-build "Run make clean && make run"
    taskdone debug-build
    taskreopen debug-build

Supported status values are free-form strings, but common values are:

    open
    active
    blocked
    done


## Task-aware workspace buttons

Workspace button actions can update task files directly.

Examples of button actions:

    shell:taskstatus debug-build active
    shell:taskdone debug-build
    shell:tasknext debug-build "Run make clean && make run"
    shell:taskreopen debug-build

Generated task workspaces include task lifecycle buttons such as:

    Mark Active
    Mark Done


## Task event log

Task files store lifecycle events.

Examples:

    tasklog debug-build
    taskstatus debug-build active
    tasknext debug-build "Run make clean && make run"
    taskdone debug-build
    tasklog debug-build

Task files include event lines:

    EVENT=created
    EVENT=status active
    EVENT=next Run make clean && make run
    EVENT=status done
