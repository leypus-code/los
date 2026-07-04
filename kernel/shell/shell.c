#include "../include/shell.h"
#include "../include/terminal.h"
#include "../include/string.h"
#include "../include/kernel.h"
#include "../include/timer.h"
#include "../include/kprintf.h"
#include "../include/theme.h"
#include "../include/keycodes.h"
#include "../include/pager.h"
#include "../include/memory.h"
#include "../include/task.h"
#include "../include/vfs.h"
#include "../include/rtc.h"
#include "../include/keyboard.h"
#include "../include/app.h"
#include "../include/ipc.h"
#include "../include/eventlog.h"
#include "../include/loader.h"
#include "../include/package.h"
#include "../include/editor.h"
#include "../include/ui.h"
#include "../include/wm.h"
#include "../include/layout.h"
#include "../include/fileassoc.h"
#include "../include/workspace_builder.h"
#include "../include/version.h"
#include "../include/ai.h"
#include "../include/model.h"
#include "../include/service.h"
#include "../include/intent.h"
#include "../include/norton.h"
#include "../include/pmm.h"
#include "../include/paging.h"

#define SHELL_BUFFER_SIZE 512

static char input_buffer[SHELL_BUFFER_SIZE];
static int input_length = 0;
static int input_cursor = 0;
static int input_scroll = 0;
static int input_start_x = 0;
static int input_start_y = 0;

#define SHELL_REDIRECT_BUFFER_SIZE 4096
static char redirect_buffer[SHELL_REDIRECT_BUFFER_SIZE];

#define HISTORY_SIZE 32
static char command_history[HISTORY_SIZE][SHELL_BUFFER_SIZE];
static int history_count = 0;
static int history_view = -1;

#define CAT_MAX_LINES 32
static const char *cat_lines[CAT_MAX_LINES];

#define TREE_MAX_LINES 160
#define TREE_LINE_LEN 80
static char tree_line_storage[TREE_MAX_LINES][TREE_LINE_LEN];
static const char *tree_lines[TREE_MAX_LINES];
static int tree_line_count = 0;
static int tree_truncated = 0;

static void *last_alloc = 0;
static int ui_mode = 0;
static vfs_node_t *shell_cwd = 0;

static void shell_redraw_input(void);
static void shell_set_input(const char *cmd);
static void shell_prompt_newline(void);

int shell_is_ui_mode(void) { return ui_mode != 0; }

int shell_get_ui_mode(void) { return ui_mode; }

void shell_set_ui_mode(int mode) { ui_mode = mode; }


static void shell_status_line(const char *tag, const char *text, uint8_t color) {
    terminal_set_color(color);
    terminal_writestring(tag);

    theme_set_normal();
    terminal_writestring(text);
    terminal_writestring("\n");
}

static void shell_ok(const char *text) {
    shell_status_line("[OK] ", text, theme_color_ok());
}

static void shell_error(const char *text) {
    shell_status_line("[ERR] ", text, theme_color_error());
}

static void shell_info(const char *text) {
    shell_status_line("[INFO] ", text, theme_color_info());
}

static void shell_print_path(void) {
    char path[64];
    if (!shell_cwd) shell_cwd = vfs_get_root();
    vfs_path(shell_cwd, path, 64);
    terminal_writestring(path);
}


static void shell_copy(char *dst, const char *src) {
    int i = 0;
    while (src[i] && i < SHELL_BUFFER_SIZE - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void shell_save_history(const char *cmd) {
    if (!cmd || !cmd[0]) return;

    int slot = history_count % HISTORY_SIZE;
    shell_copy(command_history[slot], cmd);
    history_count++;
    history_view = -1;
}


static void shell_print_history(void) {
    int start = history_count - HISTORY_SIZE;

    if (start < 0) {
        start = 0;
    }

    if (history_count == 0) {
        kprintf("History empty\n");
        return;
    }

    kprintf("Command history:\n");

    for (int i = start; i < history_count; i++) {
        int slot = i % HISTORY_SIZE;
        kprintf("  %u  %s\n", (uint32_t)(i + 1), command_history[slot]);
    }
}



static const char *completion_commands[] = {
    "help", "commands", "history", "clear", "version", "uptime", "time", "date", "clock",
    "echo", "pwd", "uname", "whoami", "hostname", "true", "false",
    "ls", "tree", "cd", "cat", "write", "mkdir", "touch", "rm", "rename", "cp", "mv",
    "nano", "edit", "nc", "wm", "currentapp",
    "themes", "theme",
    "workspaces", "open", "mkworkspace", "workspace", "workstatus",
    "wsblocks", "wsremove", "wsreplace", "wsaction", "wstemplate", "wstitle", "wsadd", "wsbutton", "wsnode", "wsend",
    "run", "startup", "intent", "gentask", "tasks", "tasklist", "taskshow", "tasklog", "taskstatus", "tasknext", "taskreopen", "taskdone", "taskopen", "ai", "aistatus", "services", "service", "apps", "runapp", "handlers",
    "models", "modelstatus", "importmodel", "loadmodel",
    "packages", "install", "remove", "formats", "load",
    "mem", "pages", "paging", "kmalloc", "kfree", "allocpage", "freepage",
    "ps", "newtask", "current", "schedule", "dmesg", "kbd", "panic",
    "scrollup", "scrolldown", "top", "bottom"
};

#define COMPLETION_COMMAND_COUNT ((int)(sizeof(completion_commands) / sizeof(completion_commands[0])))


static int shell_starts_with(const char *s, const char *prefix) {
    int i = 0;

    if (!s || !prefix) return 0;

    while (prefix[i]) {
        if (s[i] != prefix[i]) return 0;
        i++;
    }

    return 1;
}

static int shell_strlen_local(const char *s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

static void shell_insert_text_at_cursor(const char *text) {
    int add = shell_strlen_local(text);

    if (!text || add <= 0) return;

    if (input_length + add >= SHELL_BUFFER_SIZE) {
        add = SHELL_BUFFER_SIZE - 1 - input_length;
    }

    if (add <= 0) return;

    for (int i = input_length; i >= input_cursor; i--) {
        input_buffer[i + add] = input_buffer[i];
    }

    for (int i = 0; i < add; i++) {
        input_buffer[input_cursor + i] = text[i];
    }

    input_length += add;
    input_cursor += add;
    input_buffer[input_length] = '\0';

    shell_redraw_input();
}

static void shell_replace_current_word(int word_start, int word_end, const char *replacement) {
    char newbuf[SHELL_BUFFER_SIZE];
    int out = 0;

    if (!replacement) return;
    if (word_start < 0) word_start = 0;
    if (word_end < word_start) word_end = word_start;

    for (int i = 0; i < word_start && out < SHELL_BUFFER_SIZE - 1; i++) {
        newbuf[out++] = input_buffer[i];
    }

    for (int i = 0; replacement[i] && out < SHELL_BUFFER_SIZE - 1; i++) {
        newbuf[out++] = replacement[i];
    }

    int after_start = word_end;
    while (input_buffer[after_start] && out < SHELL_BUFFER_SIZE - 1) {
        newbuf[out++] = input_buffer[after_start++];
    }

    newbuf[out] = '\0';

    int repl_len = shell_strlen_local(replacement);

    shell_copy(input_buffer, newbuf);
    input_length = shell_strlen_local(input_buffer);
    input_cursor = word_start + repl_len;

    if (input_cursor > input_length) input_cursor = input_length;

    shell_redraw_input();
}


static int shell_complete_path_word(int word_start, int word_end) {
    char word[96];
    char parent_path[96];
    char prefix[48];
    char completed[96];
    int word_len = 0;
    int slash = -1;

    for (int i = word_start; i < word_end && word_len < 95; i++) {
        word[word_len++] = input_buffer[i];
    }
    word[word_len] = '\0';

    for (int i = 0; i < word_len; i++) {
        if (word[i] == '/') slash = i;
    }

    if (slash < 0) {
        parent_path[0] = '\0';
        int i = 0;
        while (word[i] && i < 47) {
            prefix[i] = word[i];
            i++;
        }
        prefix[i] = '\0';
    } else if (slash == 0) {
        parent_path[0] = '/';
        parent_path[1] = '\0';

        int pi = 0;
        int wi = 1;
        while (word[wi] && pi < 47) {
            prefix[pi++] = word[wi++];
        }
        prefix[pi] = '\0';
    } else {
        int i = 0;
        while (i < slash && i < 95) {
            parent_path[i] = word[i];
            i++;
        }
        parent_path[i] = '\0';

        int pi = 0;
        int wi = slash + 1;
        while (word[wi] && pi < 47) {
            prefix[pi++] = word[wi++];
        }
        prefix[pi] = '\0';
    }

    vfs_node_t *parent = 0;

    if (slash < 0) {
        parent = shell_cwd;
    } else {
        parent = vfs_resolve(shell_cwd, parent_path);
    }

    if (!parent || parent->type != VFS_DIRECTORY) {
        return 0;
    }

    vfs_node_t *match = 0;
    int matches = 0;

    vfs_node_t *child = parent->children;
    while (child) {
        if (shell_starts_with(child->name, prefix)) {
            match = child;
            matches++;
        }

        child = child->next;
    }

    if (matches == 0) {
        return 0;
    }

    if (matches == 1 && match) {
        int out = 0;

        if (slash < 0) {
            completed[0] = '\0';
        } else {
            for (int i = 0; i <= slash && out < 95; i++) {
                completed[out++] = word[i];
            }
            completed[out] = '\0';
        }

        for (int i = 0; match->name[i] && out < 95; i++) {
            completed[out++] = match->name[i];
        }

        if (match->type == VFS_DIRECTORY && out < 95) {
            completed[out++] = '/';
        }

        completed[out] = '\0';

        shell_replace_current_word(word_start, word_end, completed);
        return 1;
    }

    terminal_writestring("\n");

    child = parent->children;
    while (child) {
        if (shell_starts_with(child->name, prefix)) {
            terminal_writestring(child->name);
            if (child->type == VFS_DIRECTORY) {
                terminal_writestring("/");
            }
            terminal_writestring("  ");
        }

        child = child->next;
    }

    shell_prompt_newline();
    shell_set_input(input_buffer);

    return 1;
}


static int shell_complete_command_word(int word_start, int word_end) {
    char prefix[48];
    int plen = 0;
    const char *match = 0;
    int matches = 0;

    for (int i = word_start; i < word_end && plen < 47; i++) {
        prefix[plen++] = input_buffer[i];
    }
    prefix[plen] = '\0';

    for (int i = 0; i < COMPLETION_COMMAND_COUNT; i++) {
        if (shell_starts_with(completion_commands[i], prefix)) {
            match = completion_commands[i];
            matches++;
        }
    }

    if (matches == 0) return 0;

    if (matches == 1 && match) {
        shell_replace_current_word(word_start, word_end, match);
        shell_insert_text_at_cursor(" ");
        return 1;
    }

    terminal_writestring("\n");

    for (int i = 0; i < COMPLETION_COMMAND_COUNT; i++) {
        if (shell_starts_with(completion_commands[i], prefix)) {
            terminal_writestring(completion_commands[i]);
            terminal_writestring("  ");
        }
    }

    shell_prompt_newline();
    shell_set_input(input_buffer);

    return 1;
}

static int shell_complete_theme_word(int word_start, int word_end) {
    char prefix[48];
    int plen = 0;
    const char *match = 0;
    int matches = 0;

    for (int i = word_start; i < word_end && plen < 47; i++) {
        prefix[plen++] = input_buffer[i];
    }
    prefix[plen] = '\0';

    int count = theme_count();

    for (int i = 0; i < count; i++) {
        const char *name = theme_name_at(i);

        if (shell_starts_with(name, prefix)) {
            match = name;
            matches++;
        }
    }

    if (matches == 0) return 0;

    if (matches == 1 && match) {
        shell_replace_current_word(word_start, word_end, match);
        return 1;
    }

    terminal_writestring("\n");

    for (int i = 0; i < count; i++) {
        const char *name = theme_name_at(i);

        if (shell_starts_with(name, prefix)) {
            terminal_writestring(name);
            terminal_writestring("  ");
        }
    }

    shell_prompt_newline();
    shell_set_input(input_buffer);

    return 1;
}


static void shell_handle_tab(void) {
    int word_start = input_cursor;
    int word_end = input_cursor;
    int is_first_word = 0;

    while (word_start > 0 && input_buffer[word_start - 1] != ' ') {
        word_start--;
    }

    while (word_end < input_length && input_buffer[word_end] != ' ') {
        word_end++;
    }

    is_first_word = word_start == 0;

    if (is_first_word) {
        if (shell_complete_command_word(word_start, word_end)) {
            return;
        }

        return;
    }

    if (shell_starts_with(input_buffer, "theme ")) {
        if (shell_complete_theme_word(word_start, word_end)) {
            return;
        }

        return;
    }

    if (shell_complete_path_word(word_start, word_end)) {
        return;
    }
}

static void shell_redraw_input(void) {
    uint8_t color = theme_color_normal();
    int visible_width = 80 - input_start_x;
    int visible_cursor = 0;

    if (visible_width < 1) {
        visible_width = 1;
    }

    if (input_cursor < input_scroll) {
        input_scroll = input_cursor;
    }

    if (input_cursor >= input_scroll + visible_width) {
        input_scroll = input_cursor - visible_width + 1;
    }

    if (input_scroll < 0) {
        input_scroll = 0;
    }

    for (int x = input_start_x; x < 80; x++) {
        terminal_putentry_at(' ', color, x, input_start_y);
    }

    for (int i = 0; i < visible_width; i++) {
        int src = input_scroll + i;

        if (src >= input_length) {
            break;
        }

        terminal_putentry_at(input_buffer[src], color, input_start_x + i, input_start_y);
    }

    if (input_scroll > 0 && input_start_x < 80) {
        terminal_putentry_at('<', theme_color_info(), input_start_x, input_start_y);
    }

    if (input_scroll + visible_width < input_length && 79 < 80) {
        terminal_putentry_at('>', theme_color_info(), 79, input_start_y);
    }

    visible_cursor = input_cursor - input_scroll;

    if (visible_cursor < 0) {
        visible_cursor = 0;
    }

    if (visible_cursor >= visible_width) {
        visible_cursor = visible_width - 1;
    }

    terminal_move_cursor(input_start_x + visible_cursor, input_start_y);
    theme_set_normal();
}

static void shell_clear_current_input(void) {
    input_length = 0;
    input_cursor = 0;
    input_scroll = 0;
    input_buffer[0] = '\0';
    shell_redraw_input();
}

static void shell_set_input(const char *cmd) {
    int i = 0;

    while (cmd && cmd[i] && i < SHELL_BUFFER_SIZE - 1) {
        input_buffer[i] = cmd[i];
        i++;
    }

    input_length = i;
    input_cursor = input_length;
    input_scroll = 0;
    input_buffer[input_length] = '\0';

    shell_redraw_input();
}

static void shell_history_up(void) {
    if (history_count == 0) return;

    if (history_view < 0) {
        history_view = history_count - 1;
    } else if (history_view > 0) {
        history_view--;
    }

    shell_set_input(command_history[history_view % HISTORY_SIZE]);
}

static void shell_history_down(void) {
    if (history_count == 0 || history_view < 0) return;

    if (history_view < history_count - 1) {
        history_view++;
        shell_set_input(command_history[history_view % HISTORY_SIZE]);
    } else {
        history_view = -1;
        shell_clear_current_input();
    }
}

static void shell_prompt(void);

void shell_show_prompt(void) {
    shell_prompt();
}

static void shell_prompt(void) {
    theme_prompt();
    shell_print_path();
    terminal_writestring("> ");

    input_start_x = (int)terminal_get_cursor_x();
    input_start_y = (int)terminal_get_cursor_y();
    input_cursor = input_length;
    input_scroll = 0;
}

static void shell_prompt_newline(void) {
    terminal_writestring("\n");
    shell_prompt();
}



static int pager_mode = 0;
static int pager_page = 0;


static void tree_line_clear(void) {
    tree_line_count = 0;
    tree_truncated = 0;
}

static void tree_line_add(vfs_node_t *node, int depth) {
    if (!node) return;

    if (tree_line_count >= TREE_MAX_LINES - 1) {
        tree_truncated = 1;
        return;
    }

    char *line = tree_line_storage[tree_line_count];
    int pos = 0;

    for (int i = 0; i < depth && pos < TREE_LINE_LEN - 1; i++) {
        line[pos++] = ' ';
        line[pos++] = ' ';
    }

    if (pos < TREE_LINE_LEN - 1) line[pos++] = '|';
    if (pos < TREE_LINE_LEN - 1) line[pos++] = '-';
    if (pos < TREE_LINE_LEN - 1) line[pos++] = ' ';

    const char *name = node->name;
    if (!name || !name[0]) name = "/";

    for (int i = 0; name[i] && pos < TREE_LINE_LEN - 2; i++) {
        line[pos++] = name[i];
    }

    if (node->type == VFS_DIRECTORY && pos < TREE_LINE_LEN - 2) {
        line[pos++] = '/';
    }

    line[pos] = '\0';
    tree_lines[tree_line_count] = line;
    tree_line_count++;
}

static void tree_collect(vfs_node_t *node, int depth) {
    if (!node || tree_truncated) return;

    tree_line_add(node, depth);

    if (node->type != VFS_DIRECTORY) return;

    vfs_node_t *child = node->children;

    while (child) {
        tree_collect(child, depth + 1);
        if (tree_truncated) return;
        child = child->next;
    }
}

static void shell_tree_pager(vfs_node_t *root) {
    tree_line_clear();
    tree_collect(root, 0);

    if (tree_truncated && tree_line_count < TREE_MAX_LINES) {
        tree_lines[tree_line_count] = "... tree output truncated";
        tree_line_count++;
    }

    if (tree_line_count <= 0) {
        tree_lines[0] = "(empty tree)";
        tree_line_count = 1;
    }

    pager_open("Filesystem Tree", tree_lines, tree_line_count);
}

static const char *help_lines[] = {
    "LOS Shell Help",
    "==============",
    "",
    "Core",
    "  help                  Show this help",
    "  commands              Show compact command inventory",
    "  clear                 Clear screen with active theme",
    "  version               Show LOS version",
    "  uptime                Show timer ticks",
    "  time                  Show RTC time",
    "  date                  Show RTC date",
    "  clock                 Show RTC date/time",
    "",
    "Linux-like commands",
    "  echo                  Print empty line",
    "  echo <text>           Print text",
    "  echo <text> > <file>  Write text to file",
    "  echo <text> >> <file> Append text to file",
    "  pwd                   Print current directory",
    "  uname                 Print kernel name",
    "  uname -a              Print kernel/version info",
    "  whoami                Print current user",
    "  hostname              Print system hostname",
    "  true                  No-op success command",
    "  false                 Failure stub command",
    "",
    "Filesystem",
    "  ls                    List current directory",
    "  tree                  Show filesystem tree",
    "  cd <dir>              Change directory",
    "  cat <file>            Print file content",
    "  write <file> <text>   Write text to file",
    "  mkdir <name>          Create directory",
    "  mkdir -p <path>       Create directory path",
    "  touch <name>          Create file",
    "  rm <name>             Remove file or empty directory",
    "  rm -r <path>          Remove directory tree recursively",
    "  rename <old> <new>    Rename file or directory",
    "  cp <src> <dst>        Copy file, quoted paths supported",
    "  mv <src> <dst>        Move/rename, quoted paths supported",
    "",
    "Editor / UI",
    "  nano <file>           Edit/create file",
    "  edit <file>           Edit/create file",
    "  nc                    Open Norton-style file manager",
    "  wm                    Open window manager",
    "  currentapp            Show current UI app",
    "",
    "Themes",
    "  themes                Interactive theme selector",
    "  theme                 Show current theme",
    "  theme list            List themes",
    "  theme next            Switch to next theme",
    "  theme prev            Switch to previous theme",
    "  theme <name>          Apply theme by name",
    "",
    "Workspaces",
    "  workspaces            List workspace files",
    "  open <file>           Open file by association",
    "  mkworkspace <name>    Create workspace file",
    "  workspace <kind>      Open generated workspace kind",
    "  workstatus            Show workspace/layout status",
    "  wsblocks <file>      List workspace blocks",
    "  wsremove <f> \"t\"   Remove block by title",
    "  wsreplace <f> ...    Replace block by title",
    "  wsaction <f> ...     Change block action",
    "  wstemplate <t> <file> Create workspace from template",
    "  workspace buttons     Actions can run shell:commands",
    "  wstitle <file> \"text\" Set workspace title",
    "  wsadd <file> ...      Add workspace block with quoted args",
    "  wsbutton <file> ...   Add workspace button with quoted args",
    "  wsnode <file> ...     Add workspace layout node",
    "  wsend <file>          End workspace layout node",
    "",
    "Scripts",
    "  run <file.los>        Run LOS script quietly",
    "  run -v <file.los>     Run LOS script verbose",
    "  startup               Run /scripts/startup.los",
    "",
    "AI / Services / Apps",
    "  intent \"text\"       Run rule-based intent",
    "  gentask <kind>        Generate task workspace",
    "  tasks                 List generated task files",
    "  tasklist              List task summaries",
    "  taskshow <name>       Show task file",
    "  tasklog <name>        Show task event log",
    "  taskopen <name>       Open task workspace",
    "  taskstatus <n> <s>    Set task status",
    "  tasknext <n> \"text\" Add next action",
    "  taskreopen <name>     Reopen task",
    "  taskdone <name>       Mark task done",
    "  ai <intent>           Send intent to AI/Intent Engine",
    "  aistatus              Show AI status",
    "  services              List services",
    "  service <s> <a> <arg> Call service bus",
    "  apps                  List apps",
    "  runapp <name>         Run app",
    "  handlers              List file associations",
    "",
    "Models / Packages / Loader",
    "  models                List models",
    "  modelstatus           Show model service status",
    "  importmodel <name>    Import/register model",
    "  loadmodel <name>      Load model",
    "  packages              List packages",
    "  install <name>        Install package",
    "  remove <name>         Remove package",
    "  formats               List executable/data formats",
    "  load <file>           Load file by loader",
    "",
    "Kernel / Debug",
    "  mem                   Show heap state",
    "  pages                 Show physical memory pages",
    "  paging                Show paging state",
    "  kmalloc               Allocate test heap block",
    "  kfree                 Free last test heap block",
    "  allocpage             Allocate physical page",
    "  freepage <addr>       Free physical page",
    "  ps                    List tasks",
    "  newtask               Create demo task",
    "  current               Show current task",
    "  schedule              Run scheduler tick",
    "  dmesg                 Print event log",
    "  kbd                   Show keyboard state",
    "  panic                 Trigger kernel panic",
    "",
    "Terminal scrollback",
    "  PageUp/F11            Scroll output up",
    "  PageDown/F12          Scroll output down",
    "  top                   Scroll to top",
    "  bottom                Scroll to bottom",
    "  scrollup              Scroll output up",
    "  scrolldown            Scroll output down",
    "",
    "Editing current shell line",
    "  Left/Right            Move cursor inside command",
    "  Up/Down               Browse command history",
    "  Backspace             Delete before cursor",
    "  Tab                   Complete command/path/theme",
    "  Long input            Horizontally scrolls with < > markers",
    "  Enter                 Execute command",
    "",
    "Pager controls",
    "  W/S or arrows         Scroll page",
    "  Q / Enter / Esc       Close pager"
};

#define HELP_LINE_COUNT (sizeof(help_lines) / sizeof(help_lines[0]))
#define HELP_PAGE_LINES 18

static void shell_draw_help_pager(void) {
    terminal_initialize();

    kprintf("LOS Help Pager     page %u/%u\n\n",
        (uint32_t)(pager_page + 1),
        (uint32_t)((HELP_LINE_COUNT + HELP_PAGE_LINES - 1) / HELP_PAGE_LINES)
    );

    int start = pager_page * HELP_PAGE_LINES;

    for (int i = 0; i < HELP_PAGE_LINES; i++) {
        int idx = start + i;
        if (idx >= (int)HELP_LINE_COUNT) break;
        kprintf("%s\n", help_lines[idx]);
    }

    kprintf("\n[w/s] scroll   [q/Enter/Esc] close");
}

static void shell_exit_pager(void) {
    pager_mode = 0;
}

__attribute__((unused)) static void shell_pager_key(int key) {
    int max_page = (HELP_LINE_COUNT + HELP_PAGE_LINES - 1) / HELP_PAGE_LINES - 1;

    if (key == 'q' || key == KEY_ESCAPE || key == KEY_ENTER) {
        shell_exit_pager();
        return;
    }

    if (key == 'w' || key == KEY_ARROW_UP) {
        if (pager_page > 0) pager_page--;
        shell_draw_help_pager();
        return;
    }

    if (key == 's' || key == KEY_ARROW_DOWN) {
        if (pager_page < max_page) pager_page++;
        shell_draw_help_pager();
        return;
    }
}



static void shell_execute(const char *command);

static int shell_next_word(char **cursor, char *out, int max) {
    char *p = *cursor;
    int len = 0;

    while (*p == ' ') p++;

    if (!*p) {
        out[0] = '\0';
        *cursor = p;
        return 0;
    }

    while (*p && *p != ' ' && len < max - 1) {
        out[len++] = *p;
        p++;
    }

    out[len] = '\0';

    while (*p == ' ') p++;

    *cursor = p;
    return 1;
}


static int shell_next_arg(char **cursor, char *out, int max) {
    char *p = *cursor;
    int len = 0;
    int quoted = 0;

    if (!cursor || !out || max <= 0) {
        return 0;
    }

    while (*p == ' ') {
        p++;
    }

    if (!*p) {
        out[0] = '\0';
        *cursor = p;
        return 0;
    }

    if (*p == '"') {
        quoted = 1;
        p++;
    }

    while (*p && len < max - 1) {
        if (quoted) {
            if (*p == '"') {
                p++;
                break;
            }

            if (*p == '\\' && p[1] == '"') {
                out[len++] = '"';
                p += 2;
                continue;
            }

            if (*p == '\\' && p[1] == '\\') {
                out[len++] = '\\';
                p += 2;
                continue;
            }

            out[len++] = *p;
            p++;
        } else {
            if (*p == ' ') {
                break;
            }

            out[len++] = *p;
            p++;
        }
    }

    out[len] = '\0';

    while (*p == ' ') {
        p++;
    }

    *cursor = p;
    return 1;
}

static const char *shell_rest_arg(char **cursor) {
    char *p = *cursor;

    while (*p == ' ') {
        p++;
    }

    *cursor = p;
    return p;
}


static int shell_ends_with_text(const char *s, const char *suffix) {
    int slen = 0;
    int tlen = 0;

    if (!s || !suffix) return 0;

    while (s[slen]) slen++;
    while (suffix[tlen]) tlen++;

    if (slen < tlen) return 0;

    return strcmp(s + slen - tlen, suffix) == 0;
}

static void task_make_filename(const char *name, char *out, int max) {
    int pos = 0;
    int has_suffix = 0;

    if (!out || max <= 0) return;
    out[0] = '\0';

    if (!name || !name[0]) return;

    if (shell_ends_with_text(name, ".task")) {
        has_suffix = 1;
    }

    if (name[0] == '/') {
        while (name[pos] && pos < max - 1) {
            out[pos] = name[pos];
            pos++;
        }
        out[pos] = '\0';
        return;
    }

    const char *prefix = "/workspaces/";
    for (int i = 0; prefix[i] && pos < max - 1; i++) {
        out[pos++] = prefix[i];
    }

    for (int i = 0; name[i] && pos < max - 1; i++) {
        out[pos++] = name[i];
    }

    if (!has_suffix && pos + 5 < max) {
        out[pos++] = '.';
        out[pos++] = 't';
        out[pos++] = 'a';
        out[pos++] = 's';
        out[pos++] = 'k';
    }

    out[pos] = '\0';
}

static vfs_node_t *task_resolve(const char *name) {
    char path[96];
    task_make_filename(name, path, 96);

    if (!path[0]) return 0;

    return vfs_resolve(shell_cwd, path);
}

static void task_print_summary(vfs_node_t *task) {
    if (!task || task->type != VFS_FILE || !task->content) return;

    const char *title = 0;
    const char *status = 0;
    const char *kind = 0;
    const char *workspace = 0;

    for (int i = 0; task->content[i]; i++) {
        if ((i == 0 || task->content[i - 1] == '\n') &&
            task->content[i] == 'T' &&
            task->content[i + 1] == 'I' &&
            task->content[i + 2] == 'T' &&
            task->content[i + 3] == 'L' &&
            task->content[i + 4] == 'E' &&
            task->content[i + 5] == '=') {
            title = task->content + i + 6;
        }

        if ((i == 0 || task->content[i - 1] == '\n') &&
            task->content[i] == 'S' &&
            task->content[i + 1] == 'T' &&
            task->content[i + 2] == 'A' &&
            task->content[i + 3] == 'T' &&
            task->content[i + 4] == 'U' &&
            task->content[i + 5] == 'S' &&
            task->content[i + 6] == '=') {
            status = task->content + i + 7;
        }

        if ((i == 0 || task->content[i - 1] == '\n') &&
            task->content[i] == 'K' &&
            task->content[i + 1] == 'I' &&
            task->content[i + 2] == 'N' &&
            task->content[i + 3] == 'D' &&
            task->content[i + 4] == '=') {
            kind = task->content + i + 5;
        }

        if ((i == 0 || task->content[i - 1] == '\n') &&
            task->content[i] == 'W' &&
            task->content[i + 1] == 'O' &&
            task->content[i + 2] == 'R' &&
            task->content[i + 3] == 'K' &&
            task->content[i + 4] == 'S' &&
            task->content[i + 5] == 'P' &&
            task->content[i + 6] == 'A' &&
            task->content[i + 7] == 'C' &&
            task->content[i + 8] == 'E' &&
            task->content[i + 9] == '=') {
            workspace = task->content + i + 10;
        }
    }

    kprintf("  %s", task->name);

    if (status) {
        kprintf("  status=");
        for (int i = 0; status[i] && status[i] != '\n'; i++) {
            kprintf("%c", status[i]);
        }
    }

    if (kind) {
        kprintf("  kind=");
        for (int i = 0; kind[i] && kind[i] != '\n'; i++) {
            kprintf("%c", kind[i]);
        }
    }

    if (title) {
        kprintf("  title=");
        for (int i = 0; title[i] && title[i] != '\n'; i++) {
            kprintf("%c", title[i]);
        }
    }

    if (workspace) {
        kprintf("  workspace=");
        for (int i = 0; workspace[i] && workspace[i] != '\n'; i++) {
            kprintf("%c", workspace[i]);
        }
    }

    kprintf("\n");
}

static void task_print_events(vfs_node_t *task) {
    int count = 0;

    if (!task || task->type != VFS_FILE || !task->content) {
        return;
    }

    kprintf("Task events: %s\n", task->name);

    for (int i = 0; task->content[i]; i++) {
        if ((i == 0 || task->content[i - 1] == '\n') &&
            task->content[i] == 'E' &&
            task->content[i + 1] == 'V' &&
            task->content[i + 2] == 'E' &&
            task->content[i + 3] == 'N' &&
            task->content[i + 4] == 'T' &&
            task->content[i + 5] == '=') {
            int p = i + 6;

            kprintf("  ");

            while (task->content[p] && task->content[p] != '\n') {
                kprintf("%c", task->content[p]);
                p++;
            }

            kprintf("\n");
            count++;
        }
    }

    if (count == 0) {
        kprintf("  none\n");
    }
}

static int task_extract_workspace(vfs_node_t *task, char *out, int max) {
    if (!task || !task->content || !out || max <= 0) return 0;

    out[0] = '\0';

    for (int i = 0; task->content[i]; i++) {
        if ((i == 0 || task->content[i - 1] == '\n') &&
            task->content[i] == 'W' &&
            task->content[i + 1] == 'O' &&
            task->content[i + 2] == 'R' &&
            task->content[i + 3] == 'K' &&
            task->content[i + 4] == 'S' &&
            task->content[i + 5] == 'P' &&
            task->content[i + 6] == 'A' &&
            task->content[i + 7] == 'C' &&
            task->content[i + 8] == 'E' &&
            task->content[i + 9] == '=') {
            int j = 0;
            int p = i + 10;

            while (task->content[p] && task->content[p] != '\n' && j < max - 1) {
                out[j++] = task->content[p++];
            }

            out[j] = '\0';
            return out[0] != '\0';
        }
    }

    return 0;
}

static int task_append_event(vfs_node_t *task, const char *event_text) {
    static char updated[768];
    int out = 0;

    if (!task || task->type != VFS_FILE || !task->content || !event_text || !event_text[0]) {
        return 0;
    }

    for (int i = 0; task->content[i] && out < 720; i++) {
        updated[out++] = task->content[i];
    }

    if (out > 0 && updated[out - 1] != '\n' && out < 720) {
        updated[out++] = '\n';
    }

    const char *prefix = "EVENT=";

    for (int i = 0; prefix[i] && out < 720; i++) {
        updated[out++] = prefix[i];
    }

    for (int i = 0; event_text[i] && out < 720; i++) {
        updated[out++] = event_text[i];
    }

    if (out < 720) {
        updated[out++] = '\n';
    }

    updated[out] = '\0';

    return vfs_write_file(task, updated);
}

static int task_append_status_event(vfs_node_t *task, const char *status) {
    char event[64];
    int pos = 0;
    const char *prefix = "status ";

    if (!status || !status[0]) {
        return 0;
    }

    for (int i = 0; prefix[i] && pos < 63; i++) {
        event[pos++] = prefix[i];
    }

    for (int i = 0; status[i] && pos < 63; i++) {
        event[pos++] = status[i];
    }

    event[pos] = '\0';

    return task_append_event(task, event);
}

static int task_append_next(vfs_node_t *task, const char *next_text) {
    static char updated[768];
    int out = 0;

    if (!task || task->type != VFS_FILE || !task->content || !next_text || !next_text[0]) {
        return 0;
    }

    for (int i = 0; task->content[i] && out < 720; i++) {
        updated[out++] = task->content[i];
    }

    if (out > 0 && updated[out - 1] != '\n' && out < 720) {
        updated[out++] = '\n';
    }

    const char *prefix = "NEXT=";

    for (int i = 0; prefix[i] && out < 720; i++) {
        updated[out++] = prefix[i];
    }

    for (int i = 0; next_text[i] && out < 720; i++) {
        updated[out++] = next_text[i];
    }

    if (out < 720) {
        updated[out++] = '\n';
    }

    updated[out] = '\0';

    return vfs_write_file(task, updated);
}

static int task_set_status(vfs_node_t *task, const char *new_status) {
    static char updated[768];
    int out = 0;
    int replaced = 0;

    if (!task || task->type != VFS_FILE || !task->content || !new_status) return 0;

    for (int i = 0; task->content[i] && out < 760;) {
        if ((i == 0 || task->content[i - 1] == '\n') &&
            task->content[i] == 'S' &&
            task->content[i + 1] == 'T' &&
            task->content[i + 2] == 'A' &&
            task->content[i + 3] == 'T' &&
            task->content[i + 4] == 'U' &&
            task->content[i + 5] == 'S' &&
            task->content[i + 6] == '=') {
            const char *prefix = "STATUS=";

            for (int k = 0; prefix[k] && out < 760; k++) {
                updated[out++] = prefix[k];
            }

            for (int k = 0; new_status[k] && out < 760; k++) {
                updated[out++] = new_status[k];
            }

            if (out < 760) {
                updated[out++] = '\n';
            }

            while (task->content[i] && task->content[i] != '\n') {
                i++;
            }

            if (task->content[i] == '\n') {
                i++;
            }

            replaced = 1;
            continue;
        }

        updated[out++] = task->content[i++];
    }

    if (!replaced) {
        const char *prefix = "STATUS=";

        if (out > 0 && updated[out - 1] != '\n' && out < 760) {
            updated[out++] = '\n';
        }

        for (int k = 0; prefix[k] && out < 760; k++) {
            updated[out++] = prefix[k];
        }

        for (int k = 0; new_status[k] && out < 760; k++) {
            updated[out++] = new_status[k];
        }

        if (out < 760) {
            updated[out++] = '\n';
        }
    }

    updated[out] = '\0';
    return vfs_write_file(task, updated);
}

static void shell_copy_unquoted_rest(const char *src, char *out, int max) {
    int start = 0;
    int end = 0;
    int len = 0;

    if (!out || max <= 0) {
        return;
    }

    out[0] = '\0';

    if (!src) {
        return;
    }

    while (src[start] == ' ') {
        start++;
    }

    end = start;
    while (src[end]) {
        end++;
    }

    while (end > start && src[end - 1] == ' ') {
        end--;
    }

    if (end > start + 1 && src[start] == '"' && src[end - 1] == '"') {
        start++;
        end--;
    }

    while (start < end && len < max - 1) {
        out[len++] = src[start++];
    }

    out[len] = '\0';
}


__attribute__((unused)) static void shell_print_file_content(const char *content) {
    int last = -1;

    if (!content) {
        terminal_writestring("\n");
        return;
    }

    terminal_writestring(content);

    for (int i = 0; content[i]; i++) {
        last = i;
    }

    if (last < 0 || content[last] != '\n') {
        terminal_writestring("\n");
    }
}


static int theme_selector_active = 0;
static int theme_selector_selected = 0;
static int theme_selector_scroll = 0;
static int theme_selector_original = 0;
static uint16_t theme_selector_shell_screen[80 * 25];
static int theme_selector_has_snapshot = 0;

static void theme_selector_draw(void) {
    theme_clear_screen();

    theme_set_title();
    terminal_writestring("LOS Theme Selector\n");

    int count = theme_count();
    int current = theme_current_index();
    int visible = 18;

    if (theme_selector_selected < 0) {
        theme_selector_selected = 0;
    }

    if (theme_selector_selected >= count) {
        theme_selector_selected = count - 1;
    }

    if (theme_selector_scroll > theme_selector_selected) {
        theme_selector_scroll = theme_selector_selected;
    }

    if (theme_selector_selected >= theme_selector_scroll + visible) {
        theme_selector_scroll = theme_selector_selected - visible + 1;
    }

    if (theme_selector_scroll < 0) {
        theme_selector_scroll = 0;
    }

    int max_scroll = count - visible;
    if (max_scroll < 0) max_scroll = 0;

    if (theme_selector_scroll > max_scroll) {
        theme_selector_scroll = max_scroll;
    }

    theme_set_normal();
    terminal_writestring("W/S or Arrows = preview   Enter = keep   Esc/Q = cancel\n");
    terminal_writestring("Showing ");

    kprintf("%u", (uint32_t)(theme_selector_scroll + 1));
    terminal_writestring("-");
    int last = theme_selector_scroll + visible;
    if (last > count) last = count;
    kprintf("%u", (uint32_t)last);
    terminal_writestring(" of ");
    kprintf("%u", (uint32_t)count);
    terminal_writestring("\n\n");

    for (int i = theme_selector_scroll; i < last; i++) {
        if (i == theme_selector_selected) {
            terminal_writestring("> ");
        } else {
            terminal_writestring("  ");
        }

        if (i == current) {
            theme_set_title();
            terminal_writestring("[x] ");
        } else {
            theme_set_normal();
            terminal_writestring("[ ] ");
        }

        terminal_writestring(theme_name_at(i));
        terminal_writestring(" - ");
        terminal_writestring(theme_label_at(i));
        terminal_writestring("\n");
    }

    terminal_writestring("\n");

    theme_set_title();

    if (theme_selector_scroll > 0) {
        terminal_writestring("[more above] ");
    }

    if (last < count) {
        terminal_writestring("[more below]");
    }

    terminal_writestring("\n");

    theme_set_normal();
}

static void theme_selector_open(void) {
    terminal_copy_screen_to(theme_selector_shell_screen);
    theme_selector_has_snapshot = 1;

    theme_selector_active = 1;
    theme_selector_original = theme_current_index();
    theme_selector_selected = theme_selector_original;
    theme_selector_scroll = 0;

    if (theme_selector_selected >= 18) {
        theme_selector_scroll = theme_selector_selected - 17;
    }

    theme_selector_draw();
}

static void theme_selector_cancel(void) {
    theme_set_index(theme_selector_original);
    theme_selector_active = 0;

    if (theme_selector_has_snapshot) {
        terminal_copy_screen_from(theme_selector_shell_screen);
        theme_selector_has_snapshot = 0;
        theme_repaint_screen();
    }

    shell_prompt_newline();
}

static void theme_selector_accept(void) {
    theme_selector_active = 0;

    if (theme_selector_has_snapshot) {
        terminal_copy_screen_from(theme_selector_shell_screen);
        theme_selector_has_snapshot = 0;
        theme_repaint_screen();
    }

    shell_prompt_newline();
}

static void theme_selector_preview(void) {
    if (theme_set_index(theme_selector_selected)) {
        theme_selector_draw();
    }
}

static int theme_selector_handle_key(int key) {
    if (!theme_selector_active) {
        return 0;
    }

    if (key == KEY_ESCAPE || key == 'q' || key == 'Q') {
        theme_selector_cancel();
        return 1;
    }

    if (key == KEY_ENTER) {
        theme_selector_accept();
        return 1;
    }

    if (key == 'w' || key == 'W' || key == KEY_ARROW_UP) {
        theme_selector_selected--;
        if (theme_selector_selected < 0) {
            theme_selector_selected = theme_count() - 1;
        }

        theme_selector_preview();
        return 1;
    }

    if (key == 's' || key == 'S' || key == KEY_ARROW_DOWN) {
        theme_selector_selected++;
        if (theme_selector_selected >= theme_count()) {
            theme_selector_selected = 0;
        }

        theme_selector_preview();
        return 1;
    }

    return 1;
}


static vfs_node_t *shell_get_or_create_file_path(const char *path) {
    char parent_path[96];
    char name[40];
    int last_slash = -1;
    int len = 0;

    if (!path || !path[0]) {
        return 0;
    }

    vfs_node_t *existing = vfs_resolve(shell_cwd, path);
    if (existing) {
        if (existing->type == VFS_FILE) {
            return existing;
        }

        return 0;
    }

    while (path[len]) {
        if (path[len] == '/') {
            last_slash = len;
        }
        len++;
    }

    if (last_slash < 0) {
        int i = 0;
        while (path[i] && i < 39) {
            name[i] = path[i];
            i++;
        }
        name[i] = '\0';

        return vfs_create_file(shell_cwd, name);
    }

    if (last_slash == 0) {
        parent_path[0] = '/';
        parent_path[1] = '\0';
    } else {
        int i = 0;
        while (i < last_slash && i < 95) {
            parent_path[i] = path[i];
            i++;
        }
        parent_path[i] = '\0';
    }

    int n = 0;
    int p = last_slash + 1;
    while (path[p] && n < 39) {
        name[n++] = path[p++];
    }
    name[n] = '\0';

    if (!name[0]) {
        return 0;
    }

    vfs_node_t *parent = vfs_resolve(shell_cwd, parent_path);
    if (!parent || parent->type != VFS_DIRECTORY) {
        return 0;
    }

    return vfs_create_file(parent, name);
}

static int shell_open_editor_for_path(const char *path) {
    vfs_node_t *file = shell_get_or_create_file_path(path);

    if (!file || file->type != VFS_FILE) {
        return 0;
    }

    editor_open_file(file);
    return 1;
}


static void shell_trim_copy(const char *src, char *dst, int max) {
    int start = 0;
    int end = 0;
    int out = 0;

    if (!src || !dst || max <= 0) return;

    while (src[start] == ' ') start++;

    end = start;
    while (src[end]) end++;

    while (end > start && src[end - 1] == ' ') end--;

    for (int i = start; i < end && out < max - 1; i++) {
        dst[out++] = src[i];
    }

    dst[out] = '\0';
}

static int shell_find_redirect(const char *command, int *out_pos, int *out_append) {
    if (!command || !out_pos || !out_append) return 0;

    for (int i = 0; command[i]; i++) {
        if (command[i] == '>' && command[i + 1] == '>') {
            *out_pos = i;
            *out_append = 1;
            return 1;
        }

        if (command[i] == '>') {
            *out_pos = i;
            *out_append = 0;
            return 1;
        }
    }

    return 0;
}

static int shell_try_echo_redirect(const char *command) {
    int pos = -1;
    int append = 0;
    char text[512];
    char path[128];

    if (!command) return 0;

    if (!(command[0] == 'e' && command[1] == 'c' && command[2] == 'h' && command[3] == ' ')) {
        return 0;
    }

    if (!shell_find_redirect(command, &pos, &append)) {
        return 0;
    }

    /*
     * Left side must be: echo <text>
     */
    if (pos <= 5) {
        shell_error("redirect: missing echo text");
        return 1;
    }

    int text_len = 0;
    for (int i = 5; i < pos && text_len < 511; i++) {
        text[text_len++] = command[i];
    }
    text[text_len] = '\0';

    shell_trim_copy(text, text, 512);

    char *path_cursor = (char *)(command + pos + (append ? 2 : 1));

    if (!shell_next_arg(&path_cursor, path, 128)) {
        shell_error("redirect: missing file");
        return 1;
    }

    if (!path[0]) {
        shell_error("redirect: missing file");
        return 1;
    }

    vfs_node_t *file = shell_get_or_create_file_path(path);

    if (!file || file->type != VFS_FILE) {
        shell_error("redirect: cannot open file");
        return 1;
    }

    int out = 0;

    if (append && file->content) {
        for (int i = 0; file->content[i] && out < SHELL_REDIRECT_BUFFER_SIZE - 1; i++) {
            redirect_buffer[out++] = file->content[i];
        }
    }

    for (int i = 0; text[i] && out < SHELL_REDIRECT_BUFFER_SIZE - 2; i++) {
        redirect_buffer[out++] = text[i];
    }

    if (out < SHELL_REDIRECT_BUFFER_SIZE - 2) {
        redirect_buffer[out++] = '\n';
    }

    redirect_buffer[out] = '\0';

    vfs_write_file(file, redirect_buffer);

    if (append) {
        shell_ok("Appended");
    } else {
        shell_ok("Written");
    }

    return 1;
}


static int shell_is_echo_command(const char *command) {
    if (!command) return 0;

    if (strcmp(command, "echo") == 0) return 1;

    return (
        command[0] == 'e' &&
        command[1] == 'c' &&
        command[2] == 'h' &&
        command[3] == 'o' &&
        command[4] == ' '
    );
}

static int shell_echo_redirect_inline(const char *command) {
    int redirect_pos = -1;
    int append = 0;
    char text[512];
    char path[128];

    if (!shell_is_echo_command(command)) {
        return 0;
    }

    for (int i = 0; command[i]; i++) {
        if (command[i] == '>') {
            redirect_pos = i;
            append = command[i + 1] == '>';
            break;
        }
    }

    if (redirect_pos < 0) {
        return 0;
    }

    int text_start = 4;
    if (command[4] == ' ') text_start = 5;

    int text_end = redirect_pos;
    while (text_end > text_start && command[text_end - 1] == ' ') {
        text_end--;
    }

    int ti = 0;
    for (int i = text_start; i < text_end && ti < 511; i++) {
        text[ti++] = command[i];
    }
    text[ti] = '\0';

    char *path_cursor = (char *)(command + redirect_pos + (append ? 2 : 1));

    if (!shell_next_arg(&path_cursor, path, 128)) {
        shell_error("redirect: missing file");
        return 1;
    }

    if (!path[0]) {
        shell_error("redirect: missing file");
        return 1;
    }

    vfs_node_t *file = shell_get_or_create_file_path(path);

    if (!file || file->type != VFS_FILE) {
        shell_error("redirect: cannot open file");
        return 1;
    }

    int out = 0;

    if (append && file->content) {
        for (int i = 0; file->content[i] && out < SHELL_REDIRECT_BUFFER_SIZE - 1; i++) {
            redirect_buffer[out++] = file->content[i];
        }
    }

    for (int i = 0; text[i] && out < SHELL_REDIRECT_BUFFER_SIZE - 2; i++) {
        redirect_buffer[out++] = text[i];
    }

    if (out < SHELL_REDIRECT_BUFFER_SIZE - 2) {
        redirect_buffer[out++] = '\n';
    }

    redirect_buffer[out] = '\0';
    vfs_write_file(file, redirect_buffer);

    if (append) shell_ok("Appended");
    else shell_ok("Written");

    return 1;
}


static const char *command_lines[] = {
    "Core: help commands history clear version uptime time date clock",
    "Linux-like: echo pwd uname uname -a whoami hostname true false",
    "Redirects: echo text > file | echo text >> file",
    "Filesystem: ls tree cd cat write mkdir mkdir-p touch rm rm-r rename cp mv",
    "Editor/UI: nano edit nc wm currentapp",
    "Themes: themes theme theme list theme next theme prev theme <name>",
    "Workspaces: workspaces open mkworkspace workspace workstatus wsblocks wsremove wsreplace wsaction wstemplate wstitle wsadd wsbutton wsnode wsend",
    "Scripts: run run -v startup",
    "AI/Services: intent gentask tasks tasklist taskshow tasklog taskopen taskstatus tasknext taskreopen taskdone ai aistatus services service apps runapp handlers",
    "Models/Packages: models modelstatus importmodel loadmodel packages install remove formats load",
    "Kernel/Debug: mem pages paging kmalloc kfree allocpage freepage ps newtask current schedule dmesg kbd panic",
    "Scrollback: scrollup scrolldown top bottom PageUp PageDown",
    "Line edit: Left Right Up Down Backspace Tab Enter"
};

#define COMMAND_LINE_COUNT (sizeof(command_lines) / sizeof(command_lines[0]))


static void shell_copy_node_name(vfs_node_t *node, const char *name) {
    int i = 0;

    if (!node || !name) return;

    while (name[i] && i < 31) {
        node->name[i] = name[i];
        i++;
    }

    node->name[i] = '\0';
}

static int shell_split_path(const char *path, char *parent_path, int parent_max, char *name, int name_max) {
    int len = 0;
    int last_slash = -1;

    if (!path || !path[0] || !parent_path || !name || parent_max <= 0 || name_max <= 0) {
        return 0;
    }

    while (path[len]) {
        if (path[len] == '/') {
            last_slash = len;
        }
        len++;
    }

    if (last_slash < 0) {
        parent_path[0] = '\0';

        int i = 0;
        while (path[i] && i < name_max - 1) {
            name[i] = path[i];
            i++;
        }
        name[i] = '\0';

        return name[0] != '\0';
    }

    if (last_slash == 0) {
        parent_path[0] = '/';
        parent_path[1] = '\0';
    } else {
        int i = 0;
        while (i < last_slash && i < parent_max - 1) {
            parent_path[i] = path[i];
            i++;
        }
        parent_path[i] = '\0';
    }

    int n = 0;
    int p = last_slash + 1;

    while (path[p] && n < name_max - 1) {
        name[n++] = path[p++];
    }

    name[n] = '\0';

    return name[0] != '\0';
}

static vfs_node_t *shell_resolve_parent_for_path(const char *path, char *name, int name_max) {
    char parent_path[96];

    if (!shell_split_path(path, parent_path, 96, name, name_max)) {
        return 0;
    }

    if (!parent_path[0]) {
        return shell_cwd;
    }

    return vfs_resolve(shell_cwd, parent_path);
}

static int shell_mkdir_p_path(const char *path) {
    vfs_node_t *current = 0;
    int i = 0;

    if (!path || !path[0]) {
        return 0;
    }

    if (path[0] == '/') {
        current = vfs_get_root();
        i = 1;
    } else {
        current = shell_cwd;
    }

    while (path[i]) {
        char part[32];
        int pi = 0;

        while (path[i] == '/') {
            i++;
        }

        if (!path[i]) {
            break;
        }

        while (path[i] && path[i] != '/' && pi < 31) {
            part[pi++] = path[i++];
        }

        part[pi] = '\0';

        if (!part[0]) {
            continue;
        }

        vfs_node_t *child = vfs_find_child(current, part);

        if (child) {
            if (child->type != VFS_DIRECTORY) {
                return 0;
            }

            current = child;
        } else {
            current = vfs_mkdir(current, part);

            if (!current) {
                return 0;
            }
        }

        while (path[i] == '/') {
            i++;
        }
    }

    return 1;
}

static int shell_delete_recursive(vfs_node_t *node) {
    if (!node || !node->parent) {
        return 0;
    }

    while (node->children) {
        if (!shell_delete_recursive(node->children)) {
            return 0;
        }
    }

    if (node->content) {
        kfree(node->content);
        node->content = 0;
        node->size = 0;
    }

    return vfs_delete(node);
}

static int shell_is_descendant(vfs_node_t *node, vfs_node_t *maybe_child) {
    vfs_node_t *cur = maybe_child;

    while (cur) {
        if (cur == node) {
            return 1;
        }

        cur = cur->parent;
    }

    return 0;
}

static int shell_move_node(vfs_node_t *node, vfs_node_t *new_parent, const char *new_name) {
    if (!node || !node->parent || !new_parent || new_parent->type != VFS_DIRECTORY || !new_name || !new_name[0]) {
        return 0;
    }

    if (node->type == VFS_DIRECTORY && shell_is_descendant(node, new_parent)) {
        return 0;
    }

    vfs_node_t *existing = vfs_find_child(new_parent, new_name);

    if (existing && existing != node) {
        return 0;
    }

    vfs_node_t *old_parent = node->parent;
    vfs_node_t *cur = old_parent->children;
    vfs_node_t *prev = 0;

    while (cur) {
        if (cur == node) {
            if (prev) {
                prev->next = cur->next;
            } else {
                old_parent->children = cur->next;
            }
            break;
        }

        prev = cur;
        cur = cur->next;
    }

    if (!cur) {
        return 0;
    }

    shell_copy_node_name(node, new_name);
    node->parent = new_parent;
    node->next = new_parent->children;
    new_parent->children = node;

    return 1;
}

static int shell_copy_file_command(const char *src_path, const char *dst_path) {
    vfs_node_t *src = vfs_resolve(shell_cwd, src_path);

    if (!src || src->type != VFS_FILE) {
        shell_error("cp: source file not found");
        return 1;
    }

    vfs_node_t *dst_existing = vfs_resolve(shell_cwd, dst_path);
    vfs_node_t *dst = 0;

    if (dst_existing && dst_existing->type == VFS_DIRECTORY) {
        dst = vfs_create_file(dst_existing, src->name);
    } else {
        dst = shell_get_or_create_file_path(dst_path);
    }

    if (!dst || dst->type != VFS_FILE) {
        shell_error("cp: cannot create target file");
        return 1;
    }

    if (!vfs_write_file(dst, src->content ? src->content : "")) {
        shell_error("cp: write failed");
        return 1;
    }

    shell_ok("Copied");
    return 1;
}

static int shell_move_command(const char *src_path, const char *dst_path) {
    char name[40];
    vfs_node_t *src = vfs_resolve(shell_cwd, src_path);

    if (!src || !src->parent) {
        shell_error("mv: source not found");
        return 1;
    }

    vfs_node_t *dst_existing = vfs_resolve(shell_cwd, dst_path);
    vfs_node_t *new_parent = 0;
    const char *new_name = 0;

    if (dst_existing && dst_existing->type == VFS_DIRECTORY) {
        new_parent = dst_existing;
        new_name = src->name;
    } else {
        new_parent = shell_resolve_parent_for_path(dst_path, name, 40);
        new_name = name;

        if (dst_existing) {
            shell_error("mv: target already exists");
            return 1;
        }
    }

    if (!new_parent || new_parent->type != VFS_DIRECTORY || !new_name || !new_name[0]) {
        shell_error("mv: invalid target");
        return 1;
    }

    if (!shell_move_node(src, new_parent, new_name)) {
        shell_error("mv: failed");
        return 1;
    }

    shell_ok("Moved");
    return 1;
}


static void shell_execute(const char *command) {
    terminal_writestring("\n");

    if (shell_echo_redirect_inline(command)) {
        return;
    }

    if (shell_try_echo_redirect(command)) {
        return;
    }

    if (strcmp(command, "help") == 0) {
        pager_open("LOS Help", help_lines, HELP_LINE_COUNT);
        return;
    } else if (strcmp(command, "commands") == 0) {
        pager_open("LOS Commands", command_lines, COMMAND_LINE_COUNT);
        return;
    } else if (strcmp(command, "history") == 0) {
        shell_print_history();
        return;
    } else if (strcmp(command, "themes") == 0) {
        theme_selector_open();
        return;
    } else if (strcmp(command, "theme") == 0) {
        kprintf("Current theme: %s\n", theme_current_name());
        kprintf("Use: theme list | theme next | theme prev | theme classic | theme matrix | theme amber | ...\n");
    } else if (strcmp(command, "theme list") == 0) {
        theme_list();
    } else if (strcmp(command, "theme next") == 0) {
        theme_next();
        theme_repaint_screen();
        shell_ok("Theme changed");
        kprintf("Current theme: %s\n", theme_current_name());
    } else if (strcmp(command, "theme prev") == 0) {
        theme_prev();
        theme_repaint_screen();
        shell_ok("Theme changed");
        kprintf("Current theme: %s\n", theme_current_name());
    } else if (
        command[0] == 't' &&
        command[1] == 'h' &&
        command[2] == 'e' &&
        command[3] == 'm' &&
        command[4] == 'e' &&
        command[5] == ' '
    ) {
        const char *name = command + 6;

        if (theme_set_scheme(name)) {
            theme_repaint_screen();
            shell_ok("Theme changed");
            kprintf("Current theme: %s\n", theme_current_name());
        } else {
            shell_error("Unknown theme");
        }

    } else if (strcmp(command, "version") == 0) {
        terminal_writestring(LOS_VERSION); terminal_writestring("\n");
    } else if (strcmp(command, "clear") == 0) {
        theme_clear_screen();
    } else if (strcmp(command, "uptime") == 0) {
        kprintf("Ticks: %u\n", timer_get_ticks());
    } else if (strcmp(command, "mem") == 0) {
        kprintf("Heap start:   %x\n", memory_get_heap_start());
        kprintf("Heap current: %x\n", memory_get_heap_current());
    } else if (strcmp(command, "paging") == 0) {
        kprintf("Page directory: %x\n", paging_get_directory_addr());
        kprintf("Page table 0:   %x\n", paging_get_table0_addr());
        kprintf("First entry:    %x\n", paging_get_first_entry());
        kprintf("Enabled:        %u\n", paging_is_enabled());
    } else if (strcmp(command, "kmalloc") == 0) {
        last_alloc = kmalloc(64);
        kprintf("kmalloc(64): %x\n", (uint32_t)last_alloc);
    } else if (strcmp(command, "kfree") == 0) {
        if (last_alloc) {
            kfree(last_alloc);
            kprintf("kfree: %x\n", (uint32_t)last_alloc);
            last_alloc = 0;
        } else {
            kprintf("Nothing to free\n");
        }
    } else if (strcmp(command, "pages") == 0) {
        kprintf("Total pages: %u\n", pmm_get_total_pages());
        kprintf("Used pages:  %u\n", pmm_get_used_pages());
        kprintf("Free pages:  %u\n", pmm_get_free_pages());
    } else if (strcmp(command, "allocpage") == 0) {
        void *page = pmm_alloc_page();
        kprintf("Allocated page: %x\n", (uint32_t)page);
    } else if (
        command[0] == 'f' &&
        command[1] == 'r' &&
        command[2] == 'e' &&
        command[3] == 'e' &&
        command[4] == 'p' &&
        command[5] == 'a' &&
        command[6] == 'g' &&
        command[7] == 'e' &&
        command[8] == ' '
    ) {
        uint32_t addr = atoi_hex(command + 9);
        pmm_free_page((void *)addr);
        kprintf("Freed page: %x\n", addr);

    } else if (strcmp(command, "ps") == 0) {
        task_t *task = task_get_list();

        kprintf("PID   STATE     NAME\n");

        while (task) {
            const char *state = "unknown";

            if (task->state == TASK_READY) {
                state = "ready";
            } else if (task->state == TASK_RUNNING) {
                state = "running";
            } else if (task->state == TASK_SLEEPING) {
                state = "sleeping";
            } else if (task->state == TASK_DEAD) {
                state = "dead";
            }

            kprintf("%u     %s   %s\n", task->pid, state, task->name);
            task = task->next;
        }

        kprintf("Total tasks: %u\n", task_get_count());
    } else if (strcmp(command, "newtask") == 0) {
        task_t *task = task_create("demo-task");
        if (task) {
            kprintf("Created task PID %u\n", task->pid);
        } else {
            kprintf("Failed to create task\n");
        }


    } else if (strcmp(command, "current") == 0) {
        task_t *task = task_get_current();
        if (task) {
            kprintf("Current task: PID %u (%s)\n", task->pid, task->name);
        } else {
            kprintf("No current task\n");
        }
    } else if (strcmp(command, "schedule") == 0) {
        scheduler_tick();
        task_t *task = task_get_current();
        if (task) {
            kprintf("Switched to PID %u (%s)\n", task->pid, task->name);
        }



    } else if (strcmp(command, "pwd") == 0) {
        char path[64];
        vfs_path(shell_cwd, path, 64);
        kprintf("%s\n", path);
    } else if (
        command[0] == 'c' &&
        command[1] == 'd' &&
        command[2] == ' '
    ) {
        char *rest = (char *)(command + 3);
        char path[96];

        if (!shell_next_arg(&rest, path, 96)) {
            shell_error("Usage: cd dir");
            return;
        }

        vfs_node_t *target = vfs_resolve(shell_cwd, path);

        if (target && target->type == VFS_DIRECTORY) {
            shell_cwd = target;
        } else {
            shell_error("Directory not found");
        }

    } else if (strcmp(command, "ls") == 0) {
        vfs_list(shell_cwd);
    } else if (strcmp(command, "tree") == 0) {
        shell_tree_pager(shell_cwd);
        return;

    } else if (
        command[0] == 'c' &&
        command[1] == 'a' &&
        command[2] == 't' &&
        command[3] == ' '
    ) {
        char *rest = (char *)(command + 4);
        char path[96];

        if (!shell_next_arg(&rest, path, 96)) {
            shell_error("Usage: cat file");
            return;
        }

        vfs_node_t *file = vfs_resolve(shell_cwd, path);

        if (!file || file->type != VFS_FILE) {
            shell_error("File not found");
        } else if (!file->content) {
            cat_lines[0] = "(empty file)";
            pager_open(file->name, cat_lines, 1);
            return;
        } else {
            cat_lines[0] = file->content;
            pager_open(file->name, cat_lines, 1);
            return;
        }
    } else if (
        command[0] == 'w' &&
        command[1] == 'r' &&
        command[2] == 'i' &&
        command[3] == 't' &&
        command[4] == 'e' &&
        command[5] == ' '
    ) {
        char *rest = (char *)(command + 6);
        int i = 0;

        while (rest[i] && rest[i] != ' ') {
            i++;
        }

        if (!rest[i]) {
            shell_info("Usage: write file text");
        } else {
            rest[i] = '\0';
            char *name = rest;
            char *text = rest + i + 1;

            vfs_node_t *file = vfs_find_child(shell_cwd, name);

            if (!file) {
                file = vfs_create_file(shell_cwd, name);
            }

            if (vfs_write_file(file, text)) {
                kprintf("Written %u bytes to %s\n", file->size, file->name);
            } else {
                kprintf("write failed\n");
            }
        }



    } else if (
        command[0] == 'c' &&
        command[1] == 'p' &&
        command[2] == ' '
    ) {
        char *rest = (char *)(command + 3);
        char src[64];
        char dst[64];

        if (!shell_next_arg(&rest, src, 64) || !shell_next_arg(&rest, dst, 64)) {
            shell_error("Usage: cp source target");
            return;
        }

        shell_copy_file_command(src, dst);
        return;

    } else if (
        command[0] == 'm' &&
        command[1] == 'v' &&
        command[2] == ' '
    ) {
        char *rest = (char *)(command + 3);
        char src[64];
        char dst[64];

        if (!shell_next_arg(&rest, src, 64) || !shell_next_arg(&rest, dst, 64)) {
            shell_error("Usage: mv source target");
            return;
        }

        shell_move_command(src, dst);
        return;

    } else if (
        command[0] == 'm' &&
        command[1] == 'k' &&
        command[2] == 'd' &&
        command[3] == 'i' &&
        command[4] == 'r' &&
        command[5] == ' ' &&
        command[6] == '-' &&
        command[7] == 'p' &&
        command[8] == ' '
    ) {
        char *rest = (char *)(command + 9);
        char path[96];

        if (!shell_next_arg(&rest, path, 96)) {
            shell_error("Usage: mkdir -p path");
            return;
        }

        if (!shell_mkdir_p_path(path)) {
            shell_error("mkdir -p: failed");
            return;
        }

        shell_ok("Directory path created");
        return;

    } else if (
        command[0] == 'r' &&
        command[1] == 'm' &&
        command[2] == ' ' &&
        command[3] == '-' &&
        command[4] == 'r' &&
        command[5] == ' '
    ) {
        char *rest = (char *)(command + 6);
        char path[96];

        if (!shell_next_arg(&rest, path, 96)) {
            shell_error("Usage: rm -r path");
            return;
        }

        vfs_node_t *target = vfs_resolve(shell_cwd, path);

        if (!target || !target->parent) {
            shell_error("rm -r: target not found");
            return;
        }

        if (!shell_delete_recursive(target)) {
            shell_error("rm -r: failed");
            return;
        }

        shell_ok("Removed recursively");
        return;

    } else if (strcmp(command, "scrollup") == 0) {
        terminal_scroll_up_view();
    } else if (strcmp(command, "scrolldown") == 0) {
        terminal_scroll_down_view();
    } else if (strcmp(command, "top") == 0) {
        terminal_scroll_home();
    } else if (strcmp(command, "bottom") == 0) {
        terminal_scroll_end();


    } else if (strcmp(command, "kbd") == 0 || strcmp(command, "KBD") == 0) {
        kprintf("CapsLock:      %u\n", (uint32_t)keyboard_is_capslock_on());
        kprintf("Shift down:    %u\n", (uint32_t)keyboard_is_shift_down());
        kprintf("Shift presses: %u\n", keyboard_get_shift_presses());


    } else if (
        command[0] == 'm' &&
        command[1] == 's' &&
        command[2] == 'g' &&
        command[3] == 's' &&
        command[4] == 'e' &&
        command[5] == 'n' &&
        command[6] == 'd' &&
        command[7] == ' '
    ) {
        char *rest = (char *)(command + 8);
        uint32_t pid = 0;
        int i = 0;

        while (rest[i] >= '0' && rest[i] <= '9') {
            pid = pid * 10 + (rest[i] - '0');
            i++;
        }

        if (rest[i] != ' ') {
            kprintf("Usage: msgsend pid text\n");
        } else {
            char *text = rest + i + 1;

            if (ipc_send(1, pid, 1, text)) {
                kprintf("Message sent to PID %u\n", pid);
            } else {
                kprintf("IPC queue full\n");
            }
        }
    } else if (
        command[0] == 'm' &&
        command[1] == 's' &&
        command[2] == 'g' &&
        command[3] == 'r' &&
        command[4] == 'e' &&
        command[5] == 'c' &&
        command[6] == 'v' &&
        command[7] == ' '
    ) {
        uint32_t pid = 0;
        char *rest = (char *)(command + 8);

        while (*rest >= '0' && *rest <= '9') {
            pid = pid * 10 + (*rest - '0');
            rest++;
        }

        ipc_message_t msg;

        if (ipc_receive(pid, &msg)) {
            kprintf("From PID %u: %s\n", msg.from_pid, msg.payload);
        } else {
            kprintf("No messages for PID %u\n", pid);
        }
    } else if (
        command[0] == 'm' &&
        command[1] == 's' &&
        command[2] == 'g' &&
        command[3] == 'c' &&
        command[4] == 'o' &&
        command[5] == 'u' &&
        command[6] == 'n' &&
        command[7] == 't' &&
        command[8] == ' '
    ) {
        uint32_t pid = 0;
        char *rest = (char *)(command + 9);

        while (*rest >= '0' && *rest <= '9') {
            pid = pid * 10 + (*rest - '0');
            rest++;
        }

        kprintf("Pending messages for PID %u: %u\n", pid, ipc_pending_count(pid));


    } else if (
        command[0] == 'i' &&
        command[1] == 'n' &&
        command[2] == 'b' &&
        command[3] == 'o' &&
        command[4] == 'x' &&
        command[5] == ' '
    ) {
        uint32_t pid = 0;
        char *rest = (char *)(command + 6);

        while (*rest >= '0' && *rest <= '9') {
            pid = pid * 10 + (*rest - '0');
            rest++;
        }

        ipc_print_inbox(pid);


    } else if (strcmp(command, "dmesg") == 0) {
        eventlog_print();

    } else if (strcmp(command, "currentapp") == 0) {
        kprintf("Current UI: %s\n", ui_name(ui_current()));
        kprintf("Previous UI: %s\n", ui_name(ui_previous()));

    } else if (strcmp(command, "wm") == 0) {
        wm_enter();
        return;
    } else if (strcmp(command, "wmstatus") == 0) {
        wm_status();





    } else if (
        command[0] == 'w' &&
        command[1] == 's' &&
        command[2] == 'b' &&
        command[3] == 'l' &&
        command[4] == 'o' &&
        command[5] == 'c' &&
        command[6] == 'k' &&
        command[7] == 's' &&
        command[8] == ' '
    ) {
        char *rest = (char *)(command + 9);
        char file[64];

        if (!shell_next_arg(&rest, file, 64)) {
            shell_error("Usage: wsblocks file.workspace");
            return;
        }

        if (!workspace_builder_list_blocks(file)) {
            shell_error("Workspace blocks failed");
        }

        return;

    } else if (
        command[0] == 'w' &&
        command[1] == 's' &&
        command[2] == 'r' &&
        command[3] == 'e' &&
        command[4] == 'm' &&
        command[5] == 'o' &&
        command[6] == 'v' &&
        command[7] == 'e' &&
        command[8] == ' '
    ) {
        char *rest = (char *)(command + 9);
        char file[64];
        char title[96];

        if (!shell_next_arg(&rest, file, 64) || !shell_next_arg(&rest, title, 96)) {
            shell_error("Usage: wsremove file.workspace \"Title\"");
            return;
        }

        if (workspace_builder_remove_block(file, title)) {
            shell_ok("Workspace block removed");
        } else {
            shell_error("Workspace remove failed");
        }

        return;

    } else if (
        command[0] == 'w' &&
        command[1] == 's' &&
        command[2] == 'r' &&
        command[3] == 'e' &&
        command[4] == 'p' &&
        command[5] == 'l' &&
        command[6] == 'a' &&
        command[7] == 'c' &&
        command[8] == 'e' &&
        command[9] == ' '
    ) {
        char *rest = (char *)(command + 10);
        char file[64];
        char old_title[96];
        char type[32];
        char new_title[96];
        char content[384];

        if (!shell_next_arg(&rest, file, 64) ||
            !shell_next_arg(&rest, old_title, 96) ||
            !shell_next_arg(&rest, type, 32) ||
            !shell_next_arg(&rest, new_title, 96)) {
            shell_error("Usage: wsreplace file \"Old\" type \"New\" \"Content\"");
            return;
        }

        shell_copy_unquoted_rest(rest, content, 384);

        if (!content[0]) {
            shell_error("Usage: wsreplace file \"Old\" type \"New\" \"Content\"");
            return;
        }

        if (workspace_builder_replace_block(file, old_title, type, new_title, content)) {
            shell_ok("Workspace block replaced");
        } else {
            shell_error("Workspace replace failed");
        }

        return;

    } else if (
        command[0] == 'w' &&
        command[1] == 's' &&
        command[2] == 'a' &&
        command[3] == 'c' &&
        command[4] == 't' &&
        command[5] == 'i' &&
        command[6] == 'o' &&
        command[7] == 'n' &&
        command[8] == ' '
    ) {
        char *rest = (char *)(command + 9);
        char file[64];
        char title[96];
        char action[256];

        if (!shell_next_arg(&rest, file, 64) || !shell_next_arg(&rest, title, 96)) {
            shell_error("Usage: wsaction file \"Title\" \"shell:command\"");
            return;
        }

        shell_copy_unquoted_rest(rest, action, 256);

        if (!action[0]) {
            shell_error("Usage: wsaction file \"Title\" \"shell:command\"");
            return;
        }

        if (workspace_builder_set_block_action(file, title, action)) {
            shell_ok("Workspace block action updated");
        } else {
            shell_error("Workspace action failed");
        }

        return;

    } else if (
        command[0] == 'w' &&
        command[1] == 's' &&
        command[2] == 't' &&
        command[3] == 'e' &&
        command[4] == 'm' &&
        command[5] == 'p' &&
        command[6] == 'l' &&
        command[7] == 'a' &&
        command[8] == 't' &&
        command[9] == 'e' &&
        command[10] == ' '
    ) {
        char *rest = (char *)(command + 11);
        char kind[32];
        char name[64];

        if (!shell_next_arg(&rest, kind, 32) || !shell_next_arg(&rest, name, 64)) {
            shell_error("Usage: wstemplate coding|system|notes|services file.workspace");
            return;
        }

        if (workspace_builder_template(kind, name)) {
            shell_ok("Workspace template created");
        } else {
            shell_error("Workspace template failed");
        }

        return;

    } else if (
        command[0] == 'w' &&
        command[1] == 's' &&
        command[2] == 't' &&
        command[3] == 'i' &&
        command[4] == 't' &&
        command[5] == 'l' &&
        command[6] == 'e' &&
        command[7] == ' '
    ) {
        char *rest = (char *)(command + 8);
        char name[64];
        char title_arg[96];

        if (!shell_next_arg(&rest, name, 64) || !shell_next_arg(&rest, title_arg, 96)) {
            kprintf("Usage: wstitle file.workspace \"Title\"\n");
            return;
        }

        if (workspace_builder_set_title(name, title_arg)) {
            kprintf(""); shell_ok("Workspace title set");
        } else {
            shell_error("Failed to set workspace title");
        }

        return;

    } else if (
        command[0] == 'w' &&
        command[1] == 's' &&
        command[2] == 'a' &&
        command[3] == 'd' &&
        command[4] == 'd' &&
        command[5] == ' '
    ) {
        char *rest = (char *)(command + 6);
        char name[64];
        char type[32];
        char title_arg[96];
        char content_arg[160];

        if (!shell_next_arg(&rest, name, 64) ||
            !shell_next_arg(&rest, type, 32) ||
            !shell_next_arg(&rest, title_arg, 96)) {
            kprintf("Usage: wsadd file.workspace type \"Title\" \"Content\"\n");
            return;
        }

        if (!shell_next_arg(&rest, content_arg, 160)) {
            const char *remaining = shell_rest_arg(&rest);

            if (!remaining || !remaining[0]) {
                kprintf("Usage: wsadd file.workspace type \"Title\" \"Content\"\n");
                return;
            }

            int ci = 0;
            while (remaining[ci] && ci < 159) {
                content_arg[ci] = remaining[ci];
                ci++;
            }
            content_arg[ci] = '\0';
        }

        if (workspace_builder_add_block(name, type, title_arg, content_arg)) {
            kprintf(""); shell_ok("Workspace block added");
        } else {
            shell_error("Failed to add workspace block");
        }

        return;


    } else if (
        command[0] == 'w' &&
        command[1] == 's' &&
        command[2] == 'b' &&
        command[3] == 'u' &&
        command[4] == 't' &&
        command[5] == 't' &&
        command[6] == 'o' &&
        command[7] == 'n' &&
        command[8] == ' '
    ) {
        char *rest = (char *)(command + 9);
        char name[64];
        char title_arg[96];
        char label[64];
        char action[128];

        if (!shell_next_arg(&rest, name, 64) ||
            !shell_next_arg(&rest, title_arg, 96) ||
            !shell_next_arg(&rest, label, 64) ||
            !shell_next_arg(&rest, action, 128)) {
            kprintf("Usage: wsbutton file.workspace \"Title\" \"Label\" \"action\"\n");
            return;
        }

        if (workspace_builder_add_button(name, title_arg, label, action)) {
            kprintf(""); shell_ok("Workspace button added");
        } else {
            shell_error("Failed to add workspace button");
        }

        return;

    } else if (strcmp(command, "handlers") == 0) {
        fileassoc_list();
    } else if (
        command[0] == 'o' &&
        command[1] == 'p' &&
        command[2] == 'e' &&
        command[3] == 'n' &&
        command[4] == ' '
    ) {
        char *rest = (char *)(command + 5);
        char target_arg[96];

        if (!shell_next_arg(&rest, target_arg, 96)) {
            shell_error("Usage: open file");
            return;
        }

        const char *target = target_arg;
        const char *name = target;

        vfs_node_t *target_file = vfs_resolve(shell_cwd, target_arg);

        for (int i = 0; target[i]; i++) {
            if (target[i] == '/') {
                name = target + i + 1;
            }
        }

        if (target_file && target_file->type == VFS_FILE) {
            if (!fileassoc_open(name)) {
                shell_error("Open failed");
            }
            return;
        }

        if (!fileassoc_open(name)) {
            shell_error("Open failed");
        }
        return;

    } else if (
        command[0] == 'w' &&
        command[1] == 's' &&
        command[2] == 'n' &&
        command[3] == 'o' &&
        command[4] == 'd' &&
        command[5] == 'e' &&
        command[6] == ' '
    ) {
        char *rest = (char *)(command + 7);
        char file[48];
        char kind[24];
        char orientation[24];
        char weight[8];

        weight[0] = '\0';

        if (!shell_next_arg(&rest, file, 48) ||
            !shell_next_arg(&rest, kind, 24) ||
            !shell_next_arg(&rest, orientation, 24)) {
            shell_error("Usage: wsnode file.workspace root|row|column vertical|horizontal [weight]");
            return;
        }

        shell_next_arg(&rest, weight, 8);

        if (workspace_builder_add_node(file, kind, orientation, weight)) {
            shell_ok("Workspace node added");
        } else {
            shell_error("Workspace node failed");
        }

        return;

    } else if (
        command[0] == 'w' &&
        command[1] == 's' &&
        command[2] == 'e' &&
        command[3] == 'n' &&
        command[4] == 'd' &&
        command[5] == ' '
    ) {
        char *rest = (char *)(command + 6);
        char file[48];

        if (!shell_next_arg(&rest, file, 48)) {
            shell_error("Usage: wsend file.workspace");
            return;
        }

        if (workspace_builder_end_node(file)) {
            shell_ok("Workspace node closed");
        } else {
            shell_error("Workspace end failed");
        }

        return;

    } else if (
        command[0] == 'r' &&
        command[1] == 'u' &&
        command[2] == 'n' &&
        command[3] == ' '
    ) {
        char *rest = (char *)(command + 4);
        char first_arg[96];
        char path_arg[96];
        const char *path = 0;
        int verbose = 0;

        if (!shell_next_arg(&rest, first_arg, 96)) {
            shell_error("Usage: run [-v] file.los");
            return;
        }

        if (strcmp(first_arg, "-v") == 0) {
            verbose = 1;

            if (!shell_next_arg(&rest, path_arg, 96)) {
                shell_error("Usage: run -v file.los");
                return;
            }

            path = path_arg;
        } else {
            path = first_arg;
        }

        vfs_node_t *file = vfs_resolve(shell_cwd, path);

        if (!file || file->type != VFS_FILE || !file->content) {
            shell_error("Script not found");
            return;
        }

        shell_info("Running script");

        char line[256];
        int pos = 0;
        int li = 0;
        int executed = 0;
        int skipped = 0;

        while (file->content[pos]) {
            li = 0;

            while (file->content[pos] && file->content[pos] != '\n' && li < 255) {
                if (file->content[pos] != '\r') {
                    line[li++] = file->content[pos];
                }
                pos++;
            }

            line[li] = '\0';

            if (file->content[pos] == '\n') {
                pos++;
            }

            int first = 0;
            while (line[first] == ' ') {
                first++;
            }

            if (!line[first] || line[first] == '#') {
                skipped++;
                continue;
            }

            if (verbose) {
                terminal_writestring("$ ");
                terminal_writestring(line + first);
                terminal_writestring("\n");
                shell_execute(line + first);
            } else {
                terminal_set_output_muted(1);
                shell_execute(line + first);
                terminal_set_output_muted(0);
            }

            executed++;
        }

        terminal_set_output_muted(0);

        if (executed <= 0) {
            shell_info("Script was empty");
        } else {
            kprintf("[OK] %u commands executed", (uint32_t)executed);

            if (skipped > 0) {
                kprintf(" (%u skipped)", (uint32_t)skipped);
            }

            kprintf("\n");
        }

        return;

    } else if (strcmp(command, "startup") == 0) {
        shell_execute("run /scripts/startup.los");
        return;

    } else if (strcmp(command, "workspaces") == 0) {
        service_call("workspace", "list", "");
    } else if (
        command[0] == 'm' &&
        command[1] == 'k' &&
        command[2] == 'w' &&
        command[3] == 'o' &&
        command[4] == 'r' &&
        command[5] == 'k' &&
        command[6] == 's' &&
        command[7] == 'p' &&
        command[8] == 'a' &&
        command[9] == 'c' &&
        command[10] == 'e' &&
        command[11] == ' '
    ) {
        if (service_call("workspace", "create", command + 12)) {
            kprintf("Workspace file created: %s\n", command + 12);
        } else {
            kprintf("Workspace create failed\n");
        }

    } else if (strcmp(command, "workstatus") == 0) {
        layout_status();
    } else if (
        command[0] == 'w' &&
        command[1] == 'o' &&
        command[2] == 'r' &&
        command[3] == 'k' &&
        command[4] == 's' &&
        command[5] == 'p' &&
        command[6] == 'a' &&
        command[7] == 'c' &&
        command[8] == 'e' &&
        command[9] == ' '
    ) {
        const char *kind = command + 10;

        if (service_call("workspace", "open", kind)) {
            return;
        }

        kprintf("Unknown workspace: %s\n", kind);

    } else if (strcmp(command, "aistatus") == 0) {
        ai_status();


    } else if (strcmp(command, "services") == 0) {
        service_list();
    } else if (
        command[0] == 's' &&
        command[1] == 'e' &&
        command[2] == 'r' &&
        command[3] == 'v' &&
        command[4] == 'i' &&
        command[5] == 'c' &&
        command[6] == 'e' &&
        command[7] == ' '
    ) {
        const char *spec = command + 8;

        char svc[32];
        char act[32];
        const char *arg = 0;

        int i = 0;
        int j = 0;

        while (spec[i] && spec[i] != '.' && i < 31) {
            svc[i] = spec[i];
            i++;
        }
        svc[i] = '\0';

        if (spec[i] != '.') {
            kprintf("Usage: service name.action arg\n");
            return;
        }

        i++;

        while (spec[i] && spec[i] != ' ' && j < 31) {
            act[j++] = spec[i++];
        }
        act[j] = '\0';

        if (spec[i] != ' ') {
            kprintf("Usage: service name.action arg\n");
            return;
        }

        arg = spec + i + 1;

        if (service_call(svc, act, arg)) {
            kprintf("Service OK: %s.%s %s\n", svc, act, arg);
        } else {
            kprintf("Service failed: %s.%s %s\n", svc, act, arg);
        }

    } else if (strcmp(command, "modelstatus") == 0) {
        service_call("model", "status", "");
    } else if (strcmp(command, "models") == 0) {
        service_call("model", "list", "");
    } else if (
        command[0] == 'i' &&
        command[1] == 'm' &&
        command[2] == 'p' &&
        command[3] == 'o' &&
        command[4] == 'r' &&
        command[5] == 't' &&
        command[6] == 'm' &&
        command[7] == 'o' &&
        command[8] == 'd' &&
        command[9] == 'e' &&
        command[10] == 'l' &&
        command[11] == ' '
    ) {
        if (service_call("model", "import", command + 12)) {
            kprintf("Model imported: %s\n", command + 12);
        } else {
            kprintf("Model import failed\n");
        }
    } else if (
        command[0] == 'l' &&
        command[1] == 'o' &&
        command[2] == 'a' &&
        command[3] == 'd' &&
        command[4] == 'm' &&
        command[5] == 'o' &&
        command[6] == 'd' &&
        command[7] == 'e' &&
        command[8] == 'l' &&
        command[9] == ' '
    ) {
        if (service_call("model", "load", command + 10)) {
            kprintf("Model loaded: %s\n", command + 10);
        } else {
            kprintf("Model not found: %s\n", command + 10);
        }
    } else if (strcmp(command, "tasklist") == 0) {
        vfs_node_t *workspaces = vfs_resolve(shell_cwd, "/workspaces");

        if (!workspaces || workspaces->type != VFS_DIRECTORY) {
            shell_error("No /workspaces directory");
            return;
        }

        kprintf("Tasks:\n");

        vfs_node_t *child = workspaces->children;
        int count = 0;

        while (child) {
            if (child->type == VFS_FILE && shell_ends_with_text(child->name, ".task")) {
                task_print_summary(child);
                count++;
            }

            child = child->next;
        }

        if (count == 0) {
            kprintf("  none\n");
        }

        return;

    } else if (
        command[0] == 't' &&
        command[1] == 'a' &&
        command[2] == 's' &&
        command[3] == 'k' &&
        command[4] == 's' &&
        command[5] == 'h' &&
        command[6] == 'o' &&
        command[7] == 'w' &&
        command[8] == ' '
    ) {
        char *rest = (char *)(command + 9);
        char name[64];

        if (!shell_next_arg(&rest, name, 64)) {
            shell_error("Usage: taskshow name");
            return;
        }

        vfs_node_t *task = task_resolve(name);

        if (!task || task->type != VFS_FILE) {
            shell_error("Task not found");
            return;
        }

        cat_lines[0] = task->content ? task->content : "(empty task)";
        pager_open(task->name, cat_lines, 1);
        return;

    } else if (
        command[0] == 't' &&
        command[1] == 'a' &&
        command[2] == 's' &&
        command[3] == 'k' &&
        command[4] == 'd' &&
        command[5] == 'o' &&
        command[6] == 'n' &&
        command[7] == 'e' &&
        command[8] == ' '
    ) {
        char *rest = (char *)(command + 9);
        char name[64];

        if (!shell_next_arg(&rest, name, 64)) {
            shell_error("Usage: taskdone name");
            return;
        }

        vfs_node_t *task = task_resolve(name);

        if (!task || task->type != VFS_FILE) {
            shell_error("Task not found");
            return;
        }

        if (task_set_status(task, "done")) {
            task_append_status_event(task, "done");
            shell_ok("Task marked done");
        } else {
            shell_error("Task update failed");
        }

        return;

    } else if (
        command[0] == 't' &&
        command[1] == 'a' &&
        command[2] == 's' &&
        command[3] == 'k' &&
        command[4] == 'l' &&
        command[5] == 'o' &&
        command[6] == 'g' &&
        command[7] == ' '
    ) {
        char *rest = (char *)(command + 8);
        char name[64];

        if (!shell_next_arg(&rest, name, 64)) {
            shell_error("Usage: tasklog name");
            return;
        }

        vfs_node_t *task = task_resolve(name);

        if (!task || task->type != VFS_FILE) {
            shell_error("Task not found");
            return;
        }

        task_print_events(task);
        return;

    } else if (
        command[0] == 't' &&
        command[1] == 'a' &&
        command[2] == 's' &&
        command[3] == 'k' &&
        command[4] == 's' &&
        command[5] == 't' &&
        command[6] == 'a' &&
        command[7] == 't' &&
        command[8] == 'u' &&
        command[9] == 's' &&
        command[10] == ' '
    ) {
        char *rest = (char *)(command + 11);
        char name[64];
        char status[32];

        if (!shell_next_arg(&rest, name, 64) || !shell_next_arg(&rest, status, 32)) {
            shell_error("Usage: taskstatus name open|active|blocked|done");
            return;
        }

        vfs_node_t *task = task_resolve(name);

        if (!task || task->type != VFS_FILE) {
            shell_error("Task not found");
            return;
        }

        if (task_set_status(task, status)) {
            task_append_status_event(task, status);
            shell_ok("Task status updated");
        } else {
            shell_error("Task update failed");
        }

        return;

    } else if (
        command[0] == 't' &&
        command[1] == 'a' &&
        command[2] == 's' &&
        command[3] == 'k' &&
        command[4] == 'n' &&
        command[5] == 'e' &&
        command[6] == 'x' &&
        command[7] == 't' &&
        command[8] == ' '
    ) {
        char *rest = (char *)(command + 9);
        char name[64];
        char next_text[256];

        if (!shell_next_arg(&rest, name, 64)) {
            shell_error("Usage: tasknext name \"next action\"");
            return;
        }

        shell_copy_unquoted_rest(rest, next_text, 256);

        if (!next_text[0]) {
            shell_error("Usage: tasknext name \"next action\"");
            return;
        }

        vfs_node_t *task = task_resolve(name);

        if (!task || task->type != VFS_FILE) {
            shell_error("Task not found");
            return;
        }

        if (task_append_next(task, next_text)) {
            char event[192];
            int pos = 0;
            const char *prefix = "next ";

            for (int i = 0; prefix[i] && pos < 191; i++) {
                event[pos++] = prefix[i];
            }

            for (int i = 0; next_text[i] && pos < 191; i++) {
                event[pos++] = next_text[i];
            }

            event[pos] = '\0';
            task_append_event(task, event);

            shell_ok("Task next action added");
        } else {
            shell_error("Task next update failed");
        }

        return;

    } else if (
        command[0] == 't' &&
        command[1] == 'a' &&
        command[2] == 's' &&
        command[3] == 'k' &&
        command[4] == 'r' &&
        command[5] == 'e' &&
        command[6] == 'o' &&
        command[7] == 'p' &&
        command[8] == 'e' &&
        command[9] == 'n' &&
        command[10] == ' '
    ) {
        char *rest = (char *)(command + 11);
        char name[64];

        if (!shell_next_arg(&rest, name, 64)) {
            shell_error("Usage: taskreopen name");
            return;
        }

        vfs_node_t *task = task_resolve(name);

        if (!task || task->type != VFS_FILE) {
            shell_error("Task not found");
            return;
        }

        if (task_set_status(task, "open")) {
            task_append_event(task, "reopen");
            shell_ok("Task reopened");
        } else {
            shell_error("Task reopen failed");
        }

        return;

    } else if (
        command[0] == 't' &&
        command[1] == 'a' &&
        command[2] == 's' &&
        command[3] == 'k' &&
        command[4] == 'o' &&
        command[5] == 'p' &&
        command[6] == 'e' &&
        command[7] == 'n' &&
        command[8] == ' '
    ) {
        char *rest = (char *)(command + 9);
        char name[64];
        char workspace[96];

        if (!shell_next_arg(&rest, name, 64)) {
            shell_error("Usage: taskopen name");
            return;
        }

        vfs_node_t *task = task_resolve(name);

        if (!task || task->type != VFS_FILE) {
            shell_error("Task not found");
            return;
        }

        if (!task_extract_workspace(task, workspace, 96)) {
            shell_error("Task has no workspace");
            return;
        }

        if (service_call("workspace", "open", workspace)) {
            return;
        }

        const char *short_name = workspace;

        for (int i = 0; workspace[i]; i++) {
            if (workspace[i] == '/') {
                short_name = workspace + i + 1;
            }
        }

        if (short_name && short_name[0] && service_call("workspace", "open", short_name)) {
            return;
        }

        shell_error("Task workspace open failed");
        return;

    } else if (strcmp(command, "tasks") == 0) {
        vfs_node_t *workspaces = vfs_resolve(shell_cwd, "/workspaces");

        if (!workspaces || workspaces->type != VFS_DIRECTORY) {
            shell_error("No /workspaces directory");
            return;
        }

        kprintf("Task files:\n");

        vfs_node_t *child = workspaces->children;
        int count = 0;

        while (child) {
            int len = 0;
            while (child->name[len]) len++;

            if (len > 5 &&
                child->name[len - 5] == '.' &&
                child->name[len - 4] == 't' &&
                child->name[len - 3] == 'a' &&
                child->name[len - 2] == 's' &&
                child->name[len - 1] == 'k') {
                kprintf("  /workspaces/%s\n", child->name);
                count++;
            }

            child = child->next;
        }

        if (count == 0) {
            kprintf("  none\n");
        }

        return;

    } else if (
        command[0] == 'g' &&
        command[1] == 'e' &&
        command[2] == 'n' &&
        command[3] == 't' &&
        command[4] == 'a' &&
        command[5] == 's' &&
        command[6] == 'k' &&
        command[7] == ' '
    ) {
        char *rest = (char *)(command + 8);
        char kind[32];

        if (!shell_next_arg(&rest, kind, 32)) {
            shell_error("Usage: gentask debug|overview|writing|planning");
            return;
        }

        if (strcmp(kind, "debug") == 0) {
            intent_handle("debug build error");
            return;
        }

        if (strcmp(kind, "overview") == 0) {
            intent_handle("system overview");
            return;
        }

        if (strcmp(kind, "writing") == 0) {
            intent_handle("write notes");
            return;
        }

        if (strcmp(kind, "planning") == 0) {
            intent_handle("plan project");
            return;
        }

        shell_error("Unknown generated task");
        return;

    } else if (
        command[0] == 'i' &&
        command[1] == 'n' &&
        command[2] == 't' &&
        command[3] == 'e' &&
        command[4] == 'n' &&
        command[5] == 't' &&
        command[6] == ' '
    ) {
        char text[128];

        shell_copy_unquoted_rest(command + 7, text, 128);

        if (!text[0]) {
            shell_error("Usage: intent \"text\"");
            return;
        }

        if (!intent_handle(text)) {
            shell_error("Intent failed");
        }

        return;

    } else if (
        command[0] == 'a' &&
        command[1] == 'i' &&
        command[2] == ' '
    ) {
        char text[128];

        shell_copy_unquoted_rest(command + 3, text, 128);

        if (text[0]) {
            if (intent_handle(text)) {
                return;
            }
        }

        ai_prompt(command + 3);


    } else if (
        command[0] == 'i' &&
        command[1] == 'n' &&
        command[2] == 's' &&
        command[3] == 't' &&
        command[4] == 'a' &&
        command[5] == 'l' &&
        command[6] == 'l' &&
        command[7] == ' '
    ) {
        if (service_call("packages", "install", command + 8)) {
            kprintf("Package installed: %s\n", command + 8);
        } else {
            kprintf("Package install failed: %s\n", command + 8);
        }
    } else if (strcmp(command, "packages") == 0) {
        service_call("packages", "list", "");
    } else if (
        command[0] == 'r' &&
        command[1] == 'e' &&
        command[2] == 'm' &&
        command[3] == 'o' &&
        command[4] == 'v' &&
        command[5] == 'e' &&
        command[6] == ' '
    ) {
        if (service_call("packages", "remove", command + 7)) {
            kprintf("Package removed: %s\n", command + 7);
        } else {
            kprintf("Package not found: %s\n", command + 7);
        }

    } else if (strcmp(command, "formats") == 0) {
        loader_list_formats();
    } else if (
        command[0] == 'l' &&
        command[1] == 'o' &&
        command[2] == 'a' &&
        command[3] == 'd' &&
        command[4] == ' '
    ) {
        if (loader_load_lap_from_vfs(shell_cwd, command + 5)) {
            kprintf("Application loaded from %s\n", command + 5);
        } else {
            kprintf("LAP load failed: %s\n", command + 5);
        }

    } else if (strcmp(command, "apps") == 0) {
        service_call("apps", "list", "");
    } else if (
        command[0] == 'r' &&
        command[1] == 'u' &&
        command[2] == 'n' &&
        command[3] == ' '
    ) {
        if (strcmp(command + 4, "editor") == 0) {
            editor_enter();
            return;
        }

        if (!service_call("apps", "open", command + 4)) {
            shell_error("Application not found");
        }

    } else if (strcmp(command, "time") == 0) {
        rtc_time_t t;
        rtc_read_time(&t);
        kprintf("%u:%u:%u\n", t.hour, t.minute, t.second);
    } else if (strcmp(command, "date") == 0) {
        rtc_time_t t;
        rtc_read_time(&t);
        kprintf("%u.%u.20%u\n", t.day, t.month, t.year);
    } else if (strcmp(command, "clock") == 0) {
        rtc_time_t t;
        rtc_read_time(&t);
        kprintf("%u.%u.20%u %u:%u:%u\n", t.day, t.month, t.year, t.hour, t.minute, t.second);

    } else if (
        command[0] == 'n' &&
        command[1] == 'a' &&
        command[2] == 'n' &&
        command[3] == 'o' &&
        command[4] == ' '
    ) {
        if (!shell_open_editor_for_path(command + 5)) {
            shell_error("Cannot open editor file");
        }
        return;

    } else if (
        command[0] == 'e' &&
        command[1] == 'd' &&
        command[2] == 'i' &&
        command[3] == 't' &&
        command[4] == ' '
    ) {
        if (!shell_open_editor_for_path(command + 5)) {
            shell_error("Cannot open editor file");
        }
        return;

    } else if (strcmp(command, "nc") == 0) {
        norton_set_start_dir(shell_cwd);
        app_run("nc");
        return;
    } else if (
        command[0] == 'm' &&
        command[1] == 'k' &&
        command[2] == 'd' &&
        command[3] == 'i' &&
        command[4] == 'r' &&
        command[5] == ' '
    ) {
        vfs_node_t *node = vfs_mkdir(shell_cwd, command + 6);
        if (node) {
            kprintf("Directory created: %s\n", node->name);
        } else {
            shell_error("mkdir failed");
        }

    } else if (
        command[0] == 't' &&
        command[1] == 'o' &&
        command[2] == 'u' &&
        command[3] == 'c' &&
        command[4] == 'h' &&
        command[5] == ' '
    ) {
        vfs_node_t *node = vfs_create_file(shell_cwd, command + 6);
        if (node) {
            kprintf("File created: %s\n", node->name);
        } else {
            shell_error("touch failed");
        }

    } else if (strcmp(command, "echo") == 0) {
        terminal_writestring("\n");

    } else if (
        command[0] == 'e' &&
        command[1] == 'c' &&
        command[2] == 'h' &&
        command[3] == 'o' &&
        command[4] == ' '
    ) {
        terminal_writestring(command + 5);
        terminal_writestring("\n");

    } else if (strcmp(command, "uname") == 0) {
        terminal_writestring("LOS\n");

    } else if (strcmp(command, "uname -a") == 0) {
        terminal_writestring("LOS ");
        terminal_writestring(LOS_VERSION);
        terminal_writestring(" i386 debug\n");

    } else if (strcmp(command, "whoami") == 0) {
        terminal_writestring("root\n");

    } else if (strcmp(command, "hostname") == 0) {
        terminal_writestring("los\n");

    } else if (strcmp(command, "true") == 0) {
        /* no-op */

    } else if (strcmp(command, "false") == 0) {
        shell_error("false");

    } else if (strcmp(command, "panic") == 0) {
        kernel_panic("Manual panic requested from shell");
    } else if (strlen(command) == 0) {
        // empty command
    } else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(command);
        terminal_writestring("\n");
    }

}

void shell_run_command(const char *command) {
    if (!command) {
        return;
    }

    shell_execute(command);
}

void shell_initialize(void) {
    input_length = 0;
    input_cursor = 0;
    input_scroll = 0;
    input_buffer[0] = '\0';

    if (!shell_cwd) {
        shell_cwd = vfs_get_root();
    }

    terminal_writestring("Type 'help' to see commands. Startup: /scripts/startup.los");
    terminal_writestring("\n");

    vfs_node_t *startup = vfs_resolve(shell_cwd, "/scripts/startup.los");

    if (startup && startup->type == VFS_FILE) {
        shell_execute("run /scripts/startup.los");
    }

    shell_prompt_newline();
}

void shell_handle_key(int key) {
    if (theme_selector_handle_key(key)) {
        return;
    }

    if (pager_active()) {
        pager_key(key);

        if (!pager_active() && !shell_is_ui_mode()) {
            shell_prompt_newline();
        }

        return;
    }

    if (key == KEY_TAB) {
        shell_handle_tab();
        return;
    }

    if (key == KEY_ARROW_UP) {
        shell_history_up();
        return;
    }

    if (key == KEY_ARROW_DOWN) {
        shell_history_down();
        return;
    }

    if (key == KEY_ARROW_LEFT) {
        if (input_cursor > 0) {
            input_cursor--;
            shell_redraw_input();
        }
        return;
    }

    if (key == KEY_ARROW_RIGHT) {
        if (input_cursor < input_length) {
            input_cursor++;
            shell_redraw_input();
        }
        return;
    }

    if (key == KEY_PAGE_UP || key == KEY_F11 || key == KEY_SHIFT_UP) {
        terminal_scroll_up_view();
        return;
    }

    if (key == KEY_PAGE_DOWN || key == KEY_F12 || key == KEY_SHIFT_DOWN) {
        terminal_scroll_down_view();
        return;
    }

    if (key == KEY_HOME) {
        terminal_scroll_home();
        return;
    }

    if (key == KEY_END) {
        terminal_scroll_end();
        return;
    }

    if (key >= 0 && key < 256) {
        shell_putchar((char)key);
    }
}

void shell_resume_from_ui(void) {
    input_length = 0;
    input_cursor = 0;
    input_buffer[0] = '\0';

    terminal_restore_screen();
    shell_prompt_newline();
}


static void shell_trim_command_for_execute(const char *src, char *dst, int max) {
    int start = 0;
    int end = 0;
    int out = 0;

    if (!src || !dst || max <= 0) {
        return;
    }

    while (src[start] == ' ') {
        start++;
    }

    end = start;
    while (src[end]) {
        end++;
    }

    while (end > start && src[end - 1] == ' ') {
        end--;
    }

    for (int i = start; i < end && out < max - 1; i++) {
        dst[out++] = src[i];
    }

    dst[out] = '\0';
}

void shell_putchar(char c) {
    if (shell_is_ui_mode()) {
        return;
    }

    if (c == '\n') {
        char exec_command[SHELL_BUFFER_SIZE];

        input_buffer[input_length] = '\0';
        shell_trim_command_for_execute(input_buffer, exec_command, SHELL_BUFFER_SIZE);

        terminal_move_cursor(input_start_x + input_length, input_start_y);
        terminal_writestring("\n");

        if (exec_command[0]) {
            shell_save_history(exec_command);
            shell_execute(exec_command);
        }

        input_length = 0;
        input_cursor = 0;
        input_buffer[0] = '\0';

        if (!shell_is_ui_mode() && !pager_active()) {
            shell_prompt_newline();
        }

        return;
    }

    if (c == '\b') {
        if (input_cursor > 0 && input_length > 0) {
            for (int i = input_cursor - 1; i < input_length - 1; i++) {
                input_buffer[i] = input_buffer[i + 1];
            }

            input_length--;
            input_cursor--;
            input_buffer[input_length] = '\0';

            shell_redraw_input();
        }

        return;
    }

    if (c >= 32 && c < 127) {
        if (input_length >= SHELL_BUFFER_SIZE - 1) {
            return;
        }

        for (int i = input_length; i > input_cursor; i--) {
            input_buffer[i] = input_buffer[i - 1];
        }

        input_buffer[input_cursor] = c;
        input_length++;
        input_cursor++;
        input_buffer[input_length] = '\0';

        shell_redraw_input();
    }
}
