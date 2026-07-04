# LOS Changelog

## LOS v20.15b
- Fixed cat command for quoted paths
- Added quoted path parsing for cd, nano, edit, and open
- File commands now handle paths with spaces more consistently

## LOS v20.15a
- Fixed echo redirect parser for quoted target paths
- Redirects now support paths with spaces, for example echo hello > "/projects/my app/hello.txt"

## LOS v20.15
- Added shell_next_arg parser with quoted string support
- Added escaped quote support inside quoted arguments
- Updated cp, mv, mkdir -p, rm -r, run, wsnode, wsend to use quoted args
- Updated wstitle, wsadd, and wsbutton to support quoted workspace text

## LOS v20.14
- Added /scripts directory to default VFS layout
- Added default /scripts/startup.los boot script
- Added /projects, /notes, and /workspaces default directories
- Shell now automatically runs /scripts/startup.los on startup
- Added startup command to rerun the startup script manually

## LOS v20.13
- Added cp source target for file copying
- Added mv source target for moving/renaming files and directories
- Added mkdir -p path for recursive directory creation
- Added rm -r path for recursive directory removal
- Updated shell completion/help/docs for new filesystem commands

## LOS v20.12
- Added vertical scroll state to LOS editor
- Editor draw now renders from scroll_line instead of always from top of file
- Cursor is kept visible while navigating long files
- Added PageUp/PageDown scrolling in editor
- Editor footer now shows current line/column and buffer size

## LOS v20.11b
- Marked legacy workspace/terminal helper functions as intentionally unused
- Reached warning-clean kernel build after shell parser cleanup

## LOS v20.11a
- Fixed build after parser cleanup attempt
- Kept mutable shell parsers for legacy commands that split command strings in-place
- Removed const assignment error in write/ws* command parsing

## LOS v20.11
- Cleaned shell parser const-correctness warnings
- Updated shell_next_word to accept const command cursors
- Marked legacy shell pager helpers as unused
- Removed unused uiblock theme helper functions

## LOS v20.10
- Added Tab completion in shell
- Completes command names at the beginning of the input line
- Completes VFS paths for command arguments
- Completes theme names after theme command
- Multiple matches are listed without losing current input

## LOS v20.9
- Added terminal output mute mode for script execution
- run <file.los> now executes quietly and prints a compact summary
- run -v <file.los> keeps verbose per-command output
- Script runner now reports executed and skipped line counts

## LOS v20.8h
- Redesigned help output into grouped command sections
- Added echo redirect documentation to help
- Added commands pager for compact command inventory
- Kept HELP_LINE_COUNT generated automatically with sizeof(help_lines)

## LOS v20.8g
- Added inline echo redirect parser before normal echo handling
- Fixed echo text > file and echo text >> file not being intercepted
- Redirects now create/write/append VFS files from shell

## LOS v20.8f
- Fixed echo redirect handler priority
- echo text > file now routes before normal echo
- echo text >> file now routes before normal echo

## LOS v20.8e
- Added echo output redirection using >
- Added echo append redirection using >>
- Shell can now generate small scripts/files from command line
- Started filesystem-writing shell compatibility layer

## LOS v20.8d
- Added Linux-like shell core commands: echo, uname, uname -a, whoami, hostname
- Added true/false command stubs
- Started POSIX-style command compatibility layer for LOS shell

## LOS v20.8c
- Added shell input cursor state
- Left/Right arrows now move inside the current shell command
- Typing inserts at cursor position instead of always appending
- Backspace deletes before cursor and redraws the input line
- History recall now places cursor at end of recalled command

## LOS v20.8b
- Workspace UI blocks now use Theme Engine colors
- Fixed old blue/cyan/yellow VGA colors leaking into generated workspaces
- Workspace buttons, borders, active border, title and text now follow active theme

## LOS v20.8
- Added nano command as Linux-like alias for LOS editor
- Added edit command alias
- nano/edit can open existing files by path and create new files
- Editor now uses active Theme Engine colors

## LOS v20.7f
- Interactive theme selector now restores the previous shell screen instead of wiping shell history
- Theme selector Enter keeps previewed theme; Esc/Q restores original theme and shell screen
- Norton exit now forces prompt onto a fresh line to avoid leftover input artifacts like 'ng nc...'

## LOS v20.7e
- Shell status messages now use Theme Engine colors
- Fixed blue background leaking after theme changes
- clear command now clears with active theme background

## LOS v20.7d
- Added theme_repaint_screen to recolor existing shell text without wiping output
- Theme switching now updates visible shell background immediately
- Norton return path now repaints restored shell snapshot with active theme

## LOS v20.7c
- Norton Commander now reads colors from Theme Engine
- nc panels, dialogs, footer, file viewer and selection use active theme colors
- Pager/tree viewer now clears screen with active theme background

## LOS v20.7b
- Replaced 50 repetitive themes with curated high-contrast VGA themes
- Interactive theme selector now previews theme immediately on W/S or arrow movement
- Enter keeps selected theme, Esc/Q cancels and restores previous theme

## LOS v20.7a
- Changed default boot theme to terminal
- Added scroll viewport to interactive theme selector
- Theme selector now shows visible range and more-above/more-below markers

## LOS v20.7
- Expanded theme engine to 50 VGA text-mode color schemes
- Added default theme alias
- Theme changes no longer wipe shell history/output
- Norton and Workspace Layout now read colors from Theme Engine
- Prepared UI block renderer for shared theme colors

## LOS v20.6b
- Added interactive theme selector command: themes
- Theme selector supports W/S, arrow keys, Enter, Esc/Q
- Themes can now be previewed and applied from a full-screen selector

## LOS v20.6a
- Theme switching now repaints the shell screen with the new scheme
- Theme list now uses [x]/[ ] navigation markers
- Added theme next and theme prev commands

## LOS v20.6
- Added runtime theme engine
- Added theme command and theme list
- Added 10 extra color schemes plus classic
- Theme applies to shell, logs, prompt, ls, tree, pager text colors

## LOS v20.5c
- Added global shell prompt guard
- Prompt is now always rendered from a fresh line after command output
- Prevents LOS> from visually disappearing after commands like cat/run

## LOS v20.5b
- Sorted ls output alphabetically with directories first
- Fixed cat output so shell prompt does not visually disappear after files without trailing newline

## LOS v20.5a
- Made VFS create file/directory idempotent
- fs.touch no longer creates duplicate files with the same name
- Re-running scripts/buttons is now safer

## LOS v20.5
- Added LOS script runner command: run file.los
- Added /scripts/create-lab.los seed script
- Scripts can execute multiple shell commands from VFS files

## LOS v20.4a
- Added seeded lab.workspace
- Lab tree workspace is available on every boot
- Avoids manually typing long workspace authoring command sequences into QEMU

## LOS v20.4
- Added workspace tree authoring commands
- Added wsnode for NODE=root/row/column
- Added wsend for END
- Tree workspaces can now be created from LOS shell

## LOS v20.3
- Added single version source: kernel/include/version.h
- Planned workspace tree authoring commands
- Version display should no longer be hardcoded across random files

## LOS v20.2
- Added weighted recursive workspace layout
- NODE weights now control relative space distribution
- Example: NODE=column|vertical|2

## LOS v20.1
- Added recursive workspace layout tree
- root/row/column/block tree now controls block geometry
- Nested column layout works

## LOS v20.0
- Added workspace tree syntax parser
- NODE=root, NODE=row, NODE=column, END
- Safe flattening into existing ui_block_t renderer

## LOS v19.8
- Added path-aware shell commands
- cd /ai/prompts, cat workspaces/tree.workspace, relative and absolute paths fixed
- Styled shell errors as [ERR] on blue background

## LOS v19.7
- Added Norton footer status with active dir and file count

## LOS v19.6
- Fixed Norton long filename clipping
- Fixed wrapped input in mkdir/rename dialogs

## LOS v19.x
- Added workspace parser, workspace authoring commands, UI block actions
- Added tree pager and Norton scrolling
