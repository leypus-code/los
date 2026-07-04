# LOS Commands

Current stable version: LOS v20.12

## Core

    help
    commands
    clear
    version
    uptime
    time
    date
    clock

## Linux-like Commands

    echo
    echo text
    echo text > file
    echo text >> file
    pwd
    uname
    uname -a
    whoami
    hostname
    true
    false

## Filesystem

    ls
    tree
    cd path
    cat file
    write file text
    mkdir name
    mkdir -p path
    touch name
    rm name
    rm -r path
    rename old new
    cp source target
    mv source target

## Editor

    nano file
    edit file

Editor controls:

    F2        Save
    F10       Exit
    Esc       Exit
    Arrows    Move cursor
    PgUp      Scroll up
    PgDn      Scroll down

## UI

    nc
    wm
    currentapp

The nc command opens a Norton Commander-like file manager.

## Themes

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

## Scripts

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

## Workspaces

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

## Apps / Services / AI Stubs

    ai
    aistatus
    services
    service
    apps
    runapp
    handlers

## Models / Packages / Loader

    models
    modelstatus
    importmodel
    loadmodel
    packages
    install
    remove
    formats
    load

## Kernel / Debug

    mem
    pages
    paging
    kmalloc
    kfree
    allocpage
    freepage
    ps
    newtask
    current
    schedule
    dmesg
    kbd
    panic

## Terminal Scrollback

    scrollup
    scrolldown
    top
    bottom

## Shell Editing

    Left / Right     Move cursor
    Up / Down        History
    Backspace        Delete before cursor
    Tab              Complete command/path/theme
    Enter            Execute command


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


## Mutable workspace documents

Workspace files are editable UI documents. Commands can patch existing generated workspaces.

Examples:

    wsblocks /workspaces/debug-build.workspace

    wsremove /workspaces/debug-build.workspace "Build Log"

    wsreplace /workspaces/debug-build.workspace "Problem" status "Weather" "Weather widget placeholder"

    wsaction /workspaces/debug-build.workspace "Mark Done" "shell:taskdone debug-build"

After patching, reopen the workspace:

    open /workspaces/debug-build.workspace

This is the foundation for AI-driven UI mutation.


## Long shell input

The shell supports horizontal scrolling for long commands.

When the command is wider than the visible line, LOS shows markers:

    <  input is scrolled left
    >  more input exists to the right

This is useful for long workspace mutation commands such as wsreplace, wsaction, tasknext, and intent phrases.


## Shell history command

LOS keeps recent shell commands and can print them.

Example:

    history

This is useful for checking long commands such as wsreplace, wsaction, tasknext, and intent phrases.


## LOS AI Home

The `home` command opens the first MVP version of the user-facing LOS screen.

Examples:

    home
    los
    intent "home"
    intent "open home"

AI Home represents the future ring/chat UX:

    blank canvas
    AI command center
    generated workspaces
    task dashboard
    mutable workspace controls


## AI Ring state machine

LOS includes a text-mode MVP of the future AI ring UX.

Commands:

    ring
    ring idle
    ring chat
    ring listening
    ring thinking
    ring docked

Chat command:

    chat "debug build error"
    chat "system overview"
    chat "write notes"
    chat "plan project"

The chat command simulates the final UX flow:

    centered input / chat opens
    AI thinks
    intent runs
    workspace is generated
    ring docks


## AI workspace mutation

LOS can mutate the Home workspace through chat-style commands.

Examples:

    chat "add weather"
    chat "remove weather"

    chat "add checklist"
    chat "remove checklist"

    chat "add logs panel"
    chat "remove logs panel"

This is the first MVP step toward AI changing the screen while the user works.


## Persistent Home workspace

The Home workspace is now persistent.

The `home` command opens the existing Home workspace when it exists, instead of always regenerating it.

This preserves AI mutations such as:

    chat "add weather"
    chat "add checklist"
    chat "add logs panel"

The AI Ring state is also reflected in the `AI Ring` block on Home.


## AI operation log

LOS keeps a small in-memory operation log for AI/Ring activity.

Command:

    ops

Examples of logged operations:

    ring: chat
    ring: thinking
    mutation: added weather widget
    mutation: added checklist

Home includes an `AI Operations` block that reflects recent AI activity.


## AI screen modes

LOS can mutate the Home workspace into different working modes.

Examples:

    chat "build dashboard"
    chat "dashboard"
    chat "coding mode"
    chat "developer mode"
    chat "blank canvas"
    chat "reset home"

These commands mutate multiple widgets at once and represent the MVP version of AI reconfiguring the whole screen.


## Host AI Bridge

LOS can send prompts to a host-side AI bridge over COM1 serial.

Run LOS with serial AI bridge:

    make run-ai

In another terminal:

    python3 tools/ai_bridge.py --mode mock

Inside LOS:

    ask "make me a dashboard"
    ask "add weather"
    ask "switch to coding mode"

The bridge returns one LOS intent, then LOS executes it.

Ollama mode:

    python3 tools/ai_bridge.py --mode ollama --model llama3.2

The bridge calls the local Ollama API and routes the result back to LOS.


## Host Web Bridge

LOS can request web information through the host bridge.

Run LOS with serial bridge:

    make run-ai

In another terminal:

    python3 tools/ai_bridge.py --mode mock --web ddg

Inside LOS:

    web "weather in Vienna"
    web "latest linux kernel"
    web "what is docker"

The bridge returns a short one-line web result to LOS.

This is host-side internet access. Native kernel networking is a later milestone.


## AI tool routing

The `ask` command can route to tools.

Examples:

    ask "make me a dashboard"
    ask "switch to coding mode"
    ask "what is docker"
    ask "weather in Vienna"

The host AI bridge may return:

    build dashboard
    coding mode
    web:what is docker

When LOS receives `web:<query>`, it automatically calls the Host Web Bridge.


## Useful Host Web answers

The Host Web Bridge can return short useful answers.

Examples:

    ask "what is docker"
    ask "weather in Vienna"
    web "what is linux"
    web "weather in Vienna"

The bridge uses:
- wttr.in for weather-style queries
- Wikipedia REST summaries for what-is/who-is queries
- DuckDuckGo HTML/Lite fallback for general search


## Web Result widget

Web/AI bridge answers can update Home.

Examples:

    ask "what is docker"
    ask "weather in Vienna"
    web "what is linux"

LOS receives the bridge response, writes it into the `Web Result` block, and opens Home.


## LOS Chat Screen

The `screen` / `chatui` command opens the user-facing AI chat screen.

Examples:

    screen
    chatui
    ask "what is docker"
    ask "weather in Vienna"
    ask "make dashboard"

Bridge results update Chat Screen widgets, while Home remains the generated workspace.


## talk command

`talk` is the user-facing AI chat input.

Examples:

    talk "what is docker"
    talk "weather in Vienna"
    talk "make me a dashboard"
    talk "switch to coding mode"

Flow:

    talk
    ring opens chat
    Chat Screen shows thinking
    Host AI Bridge routes the request
    web/tool/intent executes
    Chat Screen or Home updates
    ring docks


## Boot Chat Screen

LOS opens the user-facing Chat Screen automatically after safe startup tasks.

Flow:

    boot
    startup.los safe tasks
    LOS Chat Screen
    Q -> developer shell

Commands:

    bootui
    bootui on
    bootui off
    screen
    chatui

This keeps shell as developer/BIOS mode while making Chat Screen the default user entry point.


## OpenAI Host Bridge mode

LOS can use a real hosted model through the host-side bridge.

Terminal 1:

    make run-ai

Terminal 2:

    export OPENAI_API_KEY="sk-..."
    export OPENAI_MODEL="gpt-5.5"
    python3 tools/ai_bridge.py --mode openai --web ddg

Inside LOS:

    talk "what is docker"
    talk "weather in Vienna"
    talk "make me a dashboard"
    talk "switch to coding mode"

The bridge asks the model to return exactly one LOS command, such as:

    build dashboard
    coding mode
    web:what is docker

LOS then executes that command or tool.


## Embedded local AI

LOS includes a small built-in local AI fallback.

It works without:
- Python bridge
- internet
- API keys
- OpenAI
- Ollama

Examples:

    talk "make me a dashboard"
    talk "switch to coding mode"
    talk "what is docker"
    talk "what is linux"
    talk "help"

Host bridge is optional:

    bridge
    bridge on
    bridge off

When host bridge is off, `talk` uses the embedded local model immediately.
When host bridge is on, LOS tries the host bridge first and falls back to local AI if unavailable.
