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
