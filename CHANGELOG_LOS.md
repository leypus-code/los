# LOS Changelog

## LOS v24.5
- Added COM1 serial driver
- Added host AI bridge socket target through QEMU serial
- Added model provider packets for mode/load/ask
- Added tools/ai_bridge.py bridge stub for future LLaMA/Ollama integration

## LOS v24.4
- Added model provider contract layer
- Added host/local/offline model modes
- Added model lifecycle status bridge to framebuffer ring states
- Added /model host, /model local, /model offline, /model load, and /ask commands

## LOS v24.3
- Added animated model state surface
- Added idle/loading/ready/thinking/drawing ring states
- Added /idle, /loading, /ready, /thinking, /drawing, and /model commands
- Ring animation now represents model lifecycle state

## LOS v24.2
- Added first minimal framebuffer workspace surface
- Added /workspace, /space, and /ws commands
- Workspace surface is the base for generated widgets, terminal, files, and future AI layouts

## LOS v24.1
- Added kernel reboot path via PS/2 controller reset command
- Added reboot/reset/bootmenu shell commands
- Legacy and Pixel modes can now return to GRUB boot menu by rebooting

## LOS v24.0
- Added GRUB boot menu with two real buffer systems
- Added kernel-gfx.elf for framebuffer Pixel AI Surface
- Added kernel-text.elf for legacy VGA/text shell and Norton stack
- Removed fake in-kernel boot selector as primary mode switch

## LOS v23.7
- Simplified framebuffer AI Surface: centered AI ring and chat area only
- Added graphical shell surface and /shell / /ai mode switching
- Added initial PS/2 mouse driver on IRQ12
- Added framebuffer mouse cursor rendering

## LOS v23.5
- Restored stable VGA text/pixel UI as the default boot surface
- Disabled framebuffer boot mode as default because PS/2 keyboard input is not reliable there yet
- Restored interrupt-driven kernel loop for normal keyboard input
- Simplified QEMU run target back to direct graphical window input

## LOS v23.4
- Added serial COM1 control plane as reliable primary input path
- make run now exposes LOS command input through terminal stdio
- Framebuffer UI remains graphical while host terminal provides keyboard input
- PS/2 keyboard remains optional fallback instead of blocking progress

## LOS v23.3
- Added robust PS/2 keyboard controller initialization
- Explicitly enables first PS/2 port and keyboard scanning
- Improved keyboard polling by draining multiple scancodes per kernel loop
- Updated QEMU launch flags to machine pc, VGA std, and en-us keymap
- This is the production keyboard bring-up path for framebuffer mode

## LOS v23.2g
- Added direct framebuffer PS/2 polling input path
- Polling input now bypasses shell dispatch and old modal UI layers
- Temporarily removed hlt from kernel_run during framebuffer input bring-up
- This isolates keyboard input from shell/UI routing issues

## LOS v23.2f
- Fixed framebuffer keyboard polling loop by avoiding hlt in graphics mode
- Routed framebuffer keyboard input before old modal UI handlers
- Added polling-side framebuffer key tick for PS/2 input diagnostics

## LOS v23.2e
- Added PS/2 keyboard polling fallback
- Keyboard input no longer depends only on IRQ1 delivery
- Refactored keyboard scancode handling into a shared handler for IRQ and polling paths
- This is intended to restore visible framebuffer input when keyboard IRQs are not firing

## LOS v23.2d
- Added direct framebuffer keyboard input path
- Framebuffer mode now bypasses old modal text UI input handling
- Added visible key tick debug indicator on graphical surface
- Enter executes the framebuffer input command and redraws the AI surface

## LOS v23.2c
- Fixed framebuffer keyboard input being blocked by old modal UI mode
- Graphical AI Surface now keeps shell input active
- shell_putchar now accepts input when framebuffer graphics is active
- Enter now clears and redraws the graphical input bar in framebuffer mode

## LOS v23.2a
- Fixed graphical input bar text overlap
- Added forced framebuffer input redraw after keyboard handling
- Added visible placeholder when input is empty
- Improved usability of graphical AI Surface input area

## LOS v23.2
- Added framebuffer pixel text renderer with 5x7 block font
- Added visible labels to graphical AI Surface
- Added visible framebuffer shell input bar
- shell input is now redrawn into the graphical framebuffer when graphics mode is active
- This makes the graphical surface usable as the primary visible UI direction

## LOS v23.1a
- Boot now opens the graphical AI Surface when framebuffer is available
- Text Pixel Boot Screen remains as fallback for non-framebuffer mode
- Fixes the issue where LOS stayed visually stuck on the framebuffer boot splash

## LOS v23.1
- Added first graphical AI chat surface in framebuffer mode
- Added visual AI ring/core, chat transcript panel, operations panel, and input bar
- Added gfxchat command
- screen1 now opens graphical AI surface when framebuffer is available
- This moves LOS UI direction from text workspaces toward real pixel-rendered AI surfaces

## LOS v23.0c
- Requested 800x600x32 framebuffer through Multiboot2 header
- Added kernel framebuffer graphics module
- Added primitive pixel operations: clear, put pixel, fill rect, loading bars
- Added first real graphical LOS boot splash
- Added gfxboot command for manually redrawing graphical boot screen
- This is the first real pixel-rendered LOS screen

## LOS v23.0b
- Added Multiboot2 tag parser
- Kernel now scans bootloader tags for framebuffer information
- Added framebuffer state getters
- Added gfxinfo shell command
- This prepares LOS for requesting and drawing into a real framebuffer

## LOS v23.0a
- Added Multiboot2 handoff from boot.S to kernel_main
- Kernel now stores Multiboot2 magic and info structure address
- Added multiboot2.h definitions
- Added mbi shell command for boot info debugging
- This prepares LOS for real framebuffer graphics mode

## LOS v22.7e
- Reverted terminal_initialize usage in Pixel Boot Screen
- Avoided VGA color/background corruption during boot screen drawing
- Pixel Boot Screen now scroll-clears old boot logs without resetting terminal state

## LOS v22.7d
- Pixel Boot Screen now clears previous boot logs before drawing
- Prevented visual boot screen from being pushed upward by earlier boot text
- Updated QEMU launch flags for zoom-to-fit GTK display when supported

## LOS v22.7c
- Compact Pixel Boot Screen to fit within VGA 80x25 text mode
- Prevented boot frame from scrolling the top of the screen away
- Preserved room for the LOS shell prompt below the visual boot surface

## LOS v22.7b
- Improved Pixel Boot Screen layout
- Centered LOS logo
- Added full-width boot frame
- Moved boot information inside the visual frame
- Kept shell prompt below the visual boot surface

## LOS v22.7
- Added Pixel Boot Screen as screen0
- Boot now shows a visual LOS splash/loading screen before Chat Screen
- Added screen0, pixels, and bootscreen commands
- Updated screen runtime list to include Pixel Boot Screen, Chat Screen, and Home/Dashboard
- This is the first visual OS-style screen layer before future framebuffer graphics

## LOS v22.6
- Added multi-screen workspace runtime
- Added screen1 for Chat Screen and screen2 for Home/Dashboard
- Added screens, nextscreen, and prevscreen commands
- User mode now opens screen1 through the screen runtime
- This is the first step toward multiple AI-managed screens instead of one modal workspace

## LOS v22.5a
- Added visible OK confirmations for UI patch operations
- replace_block patches on Chat Screen now reopen Chat Screen automatically
- Improved manual testing feedback for AI UI Patch Contract

## LOS v22.5
- Added AI UI Patch Contract v1
- Added ui:open, ui:template, ui:replace_block, and ui:add_chat_note operations
- Host AI responses can now return ui: patches that mutate LOS workspaces
- Added uipatch and uipatchhelp shell commands for manual testing
- This prepares LOS for real AI-driven mutable UI without hardcoded phrase matching

## LOS v22.4
- Added AI Provider Contract v1
- Added model command to show/select local or host AI provider
- Added aipacket command to inspect the context packet that future AI models receive
- Kept embedded local AI minimal instead of expanding hardcoded natural-language phrases
- This prepares LOS for real model integration without turning the kernel into a phrase-matching script

## LOS v22.3
- Added AI system context storage for workspace, last intent, and last answer
- Added context command
- Local AI can now answer where am I, what can I do, status, and what happened based on system context
- Home, screen, and chatreset now update AI workspace context
- This starts making the embedded model context-aware instead of pure keyword routing

## LOS v22.2a
- Fixed AI input mode fallback for unknown shell input
- Unknown natural-language input now routes into ai_bridge_talk when aimode is on
- Developer mode still restores strict shell unknown-command behavior

## LOS v22.2
- Added AI input mode enabled by default
- Unknown shell input can now fall back to local/host AI talk flow
- Added aimode, aimode on, aimode off commands
- Added say command as alias for talk
- Added dev and user commands to switch between developer shell and user AI mode
- This makes LOS feel like a chat-first operating system instead of a command shell

## LOS v22.1
- Added embedded local AI fallback inside the kernel-side AI bridge
- talk now works immediately without Python bridge, internet, API keys, Ollama, or OpenAI
- Added local routing for dashboard, coding mode, blank canvas, debug build, notes, planning, checklist, logs, Docker, Linux, LOS, and help
- Host bridge is now optional and disabled by default
- Added bridge, bridge on, and bridge off commands
- This gives LOS a built-in default model path for instant offline MVP demos

## LOS v22.0
- Added OpenAI Host Bridge mode
- Host bridge can now route LOS prompts through the OpenAI Responses API
- Added OPENAI_API_KEY / OPENAI_MODEL environment support
- Added .env.example for real model bridge setup
- Mock and Ollama modes remain available
- This is the first real hosted AI model integration path for LOS

## LOS v21.9
- Added Chat Screen quick actions for Docker, Weather, Dashboard, Coding, Blank Canvas, Home, and Transcript
- Chat Screen now works better as the default user-facing UI after boot
- Added chatreset command to regenerate and open Chat Screen
- Updated Chat Screen instructions for Tab/Enter usage and talk shell input

## LOS v21.8b
- Disabled automatic startup.los execution during boot
- startup.los remains available through the manual startup command
- Boot now opens Chat Screen directly without script blocking risk
- This prevents LOS from hanging at [INFO] Running script during boot

## LOS v21.8a
- Fixed transcript shell output printing escaped newline markers
- transcript now prints chat history line by line
- Final AI answers now replace temporary AI: thinking... lines
- Cleaned transcript workspace generation helper

## LOS v21.8
- Added in-memory Chat Screen transcript
- Conversation block now shows recent user/AI turns instead of only last result
- Center Input now shows the last user input
- Added transcript shell command for debugging chat history
- This makes talk feel like an actual chat loop

## LOS v21.7a
- Fixed talk flow opening Chat Screen before bridge/model execution
- talk now updates chat workspace with thinking state, executes AI/tool routing, then opens final result screen
- Fixed talk quoted argument parsing
- Prevents talk from blocking bridge responses behind modal workspace UI

## LOS v21.7
- LOS now boots into the user-facing Chat Screen after safe startup tasks
- Shell remains available as developer/BIOS mode after pressing Q
- Added bootui, bootui on, and bootui off commands
- This separates normal user startup from developer shell workflow

## LOS v21.6a
- Added script safety guard for interactive commands
- Scripts now skip screen/chatui/home/talk/ask/web/open/workspace commands
- Prevents startup.los from blocking boot before shell prompt

## LOS v21.6
- Added talk command as user-facing AI chat input
- talk opens/updates Chat Screen, sets Ring state, calls Host AI Bridge, then docks Ring
- Chat Screen and Home templates now suggest talk instead of ask
- This creates the first full chat-like flow: user input -> AI thinking -> tool/intent -> visible UI update

## LOS v21.5c
- Changed Chat Screen result format to compact first-line output
- Web/AI results are now visible in cramped 80-column text layout
- Last Tool Result now renders as query => answer instead of multiline User/Result format

## LOS v21.5b
- Fixed Chat Screen Last Tool Result not updating after ask/web results
- Chat Screen update now patches existing workspace instead of blindly regenerating it
- Added visible Chat Screen result update/debug messages
- Added fallback block names for result updates

## LOS v21.5a
- Improved Home and Chat Screen readability in 80-column text mode
- Replaced cramped 4-column areas with wider full-width and 2-column rows
- Added resetui, resethome, and resetchat commands for regenerating user-facing screens
- Chat Screen now prioritizes readable Conversation and Last Tool Result blocks

## LOS v21.5
- Added LOS Chat Screen workspace template
- Added screen and chatui shell commands
- Added chat screen intent aliases
- Web/AI bridge results now update Chat Screen widgets
- Ring state now updates both Home and Chat Screen
- This starts separating developer shell from user-facing AI screen

## LOS v21.4
- Added Web Result widget behavior for Home workspace
- Web Bridge responses now update the Home screen
- ask/web queries can mutate visible workspace state
- Home template now includes Web Result area
- This connects AI tool routing to visible UI mutation

## LOS v21.3b
- Added ASCII-safe sanitizing for Host Bridge responses
- Fixed weather output showing broken UTF-8/emoji in VGA text mode
- Weather answers now return kernel-safe text such as +27C instead of UTF-8 degree/emoji symbols

## LOS v21.3
- Improved Host Web Bridge answers
- Added weather query handling through wttr.in
- Added what-is/who-is style summaries through Wikipedia REST summary API
- Improved DuckDuckGo fallback parsing for titles/snippets
- ask can now route to more useful web answers through web:<query>

## LOS v21.2
- Added AI tool routing through Host AI Bridge
- ask can now receive web:<query> responses from the host model/router
- LOS automatically calls the Web Bridge when AI selects the web tool
- Updated host bridge mock/Ollama routing to support web:<query>

## LOS v21.1a
- Increased Host Bridge response timeout
- Fixed web requests timing out before host-side DuckDuckGo response arrives
- Added visible waiting messages for AI/Web bridge requests

## LOS v21.1
- Added Host Web Bridge protocol over serial
- Added web command in LOS shell
- Extended tools/ai_bridge.py with LOS_WEB_REQUEST / LOS_WEB_RESPONSE
- Added mock and DuckDuckGo Lite web modes for host-side internet access
- This gives LOS external web access through the host bridge without implementing kernel TCP/IP yet

## LOS v21.0
- Added COM1 serial driver
- Added Host AI Bridge protocol over serial
- Added ask command to send prompts to host AI bridge
- Added run-ai Makefile target using QEMU serial TCP port 7777
- Added tools/ai_bridge.py with mock and Ollama routing modes
- This is the first bridge from LOS kernel to a real external AI model runtime

## LOS v20.32
- Added AI screen modes for Home workspace
- Added dashboard mode through chat build dashboard / dashboard
- Added coding mode through chat coding mode / developer mode
- Added blank canvas mode through chat blank canvas / empty screen
- Added reset home / reset screen commands
- Screen modes mutate multiple Home widgets at once

## LOS v20.31a
- Fixed missing ops shell command handler
- Ensured ops command is available through shell and ring API

## LOS v20.31
- Added AI operation log in ring module
- Added ops shell command
- Home now includes AI Operations block
- Ring state changes and chat commands are logged
- Home workspace mutations are logged as AI operations

## LOS v20.30a
- Made Home workspace opening robust through direct workspace_builder_open fallback
- Fixed ring home to open Home directly instead of silently relying on service open
- Added visible Home/Ring open failure messages

## LOS v20.30
- Home now opens existing workspace instead of regenerating it every time
- Added persistent Home behavior for mutated widgets
- AI Ring state now updates the AI Ring block inside Home workspace
- chat flow docks the ring before opening/mutating the workspace
- Added ring home command to refresh/open Home with current ring state

## LOS v20.29a
- Fixed Home workspace template block escaping
- Fixed multiline widget content to use escaped workspace newlines
- Home mutation now opens through workspace service fallback

## LOS v20.29
- Added rule-based AI workspace mutation intents
- chat add weather mutates Home Command Center into Weather widget
- chat add checklist mutates Live Tasks into Checklist widget
- chat add logs panel mutates Workspace Engine into Logs panel
- Added restore/remove phrases for mutated Home widgets
- This is the first MVP of AI changing the visible workspace document

## LOS v20.28a
- Added ring module object to Makefile
- Added compile rule for kernel/ai/ring.c
- Removed minor warnings from intent and legacy shell parser

## LOS v20.28
- Added AI Ring state machine module
- Added ring command to inspect/set ring state
- Added chat command to simulate final ring/chat UX flow
- Ring chat expands, thinks, invokes intent, and docks after successful workspace generation
- Updated AI Home to reference ring/chat MVP commands

## LOS v20.27
- Added LOS AI Home workspace template
- Added home and los shell commands
- Added intent aliases for home, open home, start los, ai home, and main screen
- AI Home acts as the first MVP user-facing screen for the future ring/chat workspace UX

## LOS v20.26d
- Increased shell history from 8 to 32 commands
- Added history command to inspect recent shell commands
- Useful for debugging long workspace mutation commands

## LOS v20.26c
- Increased shell command buffer from 128 to 512 characters
- Increased workspace mutation argument buffers
- Increased workspace block content parsing limits
- Increased workspace text pool to support larger mutable workspace documents

## LOS v20.26b
- Added horizontal shell input scrolling for long commands
- Long commands now remain editable instead of disappearing past the right edge
- Added < and > visual markers when input is horizontally scrolled

## LOS v20.26
- Added mutable workspace document commands
- Added wsblocks to list workspace blocks
- Added wsremove to remove a block by title
- Added wsreplace to replace a block by title
- Added wsaction to update block actions
- Workspace documents can now be patched after generation

## LOS v20.25
- Added task event logging to .task files
- New task files now include EVENT=created
- taskstatus, taskdone, taskreopen, and tasknext append EVENT entries
- Added tasklog command to inspect task event history

## LOS v20.24
- Workspace buttons can directly invoke task lifecycle commands
- Generated task workspaces now include Mark Active and Mark Done buttons
- Normal workspace templates also include task-aware action buttons
- Layout footer updated to reflect task button behavior
- Workspace UI can now update matching .task files directly

## LOS v20.23
- Added taskstatus command to set arbitrary task status
- Added tasknext command to append NEXT actions to task files
- Added taskreopen command to set task status back to open
- Task files now support a basic lifecycle from shell commands

## LOS v20.22a
- Fixed taskopen to open task workspaces through workspace service
- taskopen now retries with basename when full /workspaces path fails

## LOS v20.22
- Added tasklist command for task summaries
- Added taskshow command to inspect task metadata
- Added taskopen command to open the workspace linked from a task file
- Added taskdone command to update task status to done
- Task files are now manageable from the shell

## LOS v20.21
- Generated workspaces now create matching .task files
- Task files store title, original intent, kind, workspace path, status, and next actions
- Added tasks command to list generated task files
- Task-oriented workspace flow now has persistent task metadata in VFS

## LOS v20.20
- Added generated task workspace templates
- Added debug, overview, writing, and planning generated workspace kinds
- Added intent aliases for debug build error, system overview, write notes, and plan project
- Added gentask shell command
- Moved closer to task-driven generated UI flow

## LOS v20.19b
- Fixed intent command to pass the full phrase instead of a single argument
- Fixed ai command intent pre-pass to use the full phrase
- Added debug build error intent alias for coding/debug workspace creation

## LOS v20.19a
- Fixed shell build by including intent.h
- Fixed help_lines string literal comma after ai command

## LOS v20.19
- Added workspace.template service action
- Added rule-based intent support for creating workspace templates
- Added intent command to the shell
- ai command now attempts intent handling before AI fallback
- Supported intents: create/open/create and open coding/system/notes/services workspace

## LOS v20.18
- Added workspace path normalization for /workspaces/name.workspace paths
- Workspace builder APIs now accept either name.workspace or /workspaces/name.workspace
- Updated workspace template docs to prefer full workspace paths
- Improved open command path handling before file association dispatch

## LOS v20.17
- Added public shell_run_command API
- Workspace button actions can now execute shell commands using shell:<command>
- Workspace buttons support run/open/nc/nano/edit/theme command actions
- Button UI now displays the configured action
- Updated workspace templates to use real shell actions
- Added default /scripts/build.los script stub

## LOS v20.16
- Added workspace_builder_template API
- Added wstemplate command
- Added coding, system, notes, and services workspace templates
- Workspace templates generate structured .workspace files with blocks and layout nodes

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
