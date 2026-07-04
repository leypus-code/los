#include "../include/workspace_builder.h"
#include "../include/layout.h"
#include "../include/vfs.h"
#include "../include/kprintf.h"
#include "../include/string.h"
#include "../include/eventlog.h"
#include "../include/fileassoc.h"

static vfs_node_t *workspaces_dir = 0;
static vfs_node_t *scripts_dir = 0;

#define WS_MAX_BLOCKS 24
#define WS_TEXT_POOL_SIZE 4096
#define WS_MAX_NODES 48

#define WS_NODE_ROOT   1
#define WS_NODE_ROW    2
#define WS_NODE_COLUMN 3
#define WS_NODE_BLOCK  4

typedef struct workspace_node {
    int kind;
    int parent;
    int first_child;
    int last_child;
    int next_sibling;
    int block_index;
    int weight;
} workspace_node_t;

static char ws_text_pool[WS_TEXT_POOL_SIZE];
static int ws_text_used = 0;

static vfs_node_t *ensure_dir(vfs_node_t *parent, const char *name) {
    vfs_node_t *dir = vfs_find_child(parent, name);
    if (!dir) dir = vfs_mkdir(parent, name);
    return dir;
}

static void ws_pool_reset(void) {
    ws_text_used = 0;
}

static char *ws_strdup_bounded(const char *src, int max_len) {
    if (!src) return 0;

    int len = 0;
    while (src[len] && len < max_len) len++;

    if (ws_text_used + len + 1 >= WS_TEXT_POOL_SIZE) {
        return 0;
    }

    char *dst = &ws_text_pool[ws_text_used];

    for (int i = 0; i < len; i++) {
        dst[i] = src[i];
    }

    dst[len] = '\0';
    ws_text_used += len + 1;

    return dst;
}

static int starts_with(const char *s, const char *prefix) {
    int i = 0;

    if (!s || !prefix) return 0;

    while (prefix[i]) {
        if (s[i] != prefix[i]) return 0;
        i++;
    }

    return 1;
}

static int same_text(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}


static const char *workspace_name_only(const char *name) {
    const char *last = name;

    if (!name) {
        return "";
    }

    for (int i = 0; name[i]; i++) {
        if (name[i] == '/') {
            last = name + i + 1;
        }
    }

    return last;
}

static int block_type_from_text(const char *type) {
    if (same_text(type, "status")) return UI_BLOCK_STATUS;
    if (same_text(type, "text")) return UI_BLOCK_TEXT;
    if (same_text(type, "button")) return UI_BLOCK_BUTTON;
    if (same_text(type, "terminal")) return UI_BLOCK_TERMINAL;
    if (same_text(type, "code")) return UI_BLOCK_CODE;
    if (same_text(type, "logs")) return UI_BLOCK_LOGS;
    if (same_text(type, "list")) return UI_BLOCK_LIST;
    if (same_text(type, "ai")) return UI_BLOCK_AI;

    return 0;
}

static void block_grid_position(int index, int *x, int *y, int *w, int *h) {
    static int grid[6][4] = {
        { 1,  2, 24,  6 },
        {26,  2, 27, 10 },
        {54,  2, 25, 10 },
        { 1, 13, 38,  6 },
        {40, 13, 20,  6 },
        {61, 13, 18,  6 }
    };

    int slot = index % 6;

    *x = grid[slot][0];
    *y = grid[slot][1];
    *w = grid[slot][2];
    *h = grid[slot][3];
}


__attribute__((unused)) static void block_tree_position(int row, int col, int *x, int *y, int *w, int *h) {
    if (row <= 0) {
        if (col == 0) {
            *x = 1;  *y = 2;  *w = 24; *h = 10;
            return;
        }

        if (col == 1) {
            *x = 26; *y = 2;  *w = 27; *h = 10;
            return;
        }

        *x = 54; *y = 2; *w = 25; *h = 10;
        return;
    }

    if (col == 0) {
        *x = 1;  *y = 13; *w = 38; *h = 6;
        return;
    }

    if (col == 1) {
        *x = 40; *y = 13; *w = 20; *h = 6;
        return;
    }

    *x = 61; *y = 13; *w = 18; *h = 6;
}

static void copy_token(char *dst, int max, const char *start, int len) {
    int n = 0;

    while (n < len && n < max - 1 && start[n]) {
        dst[n] = start[n];
        n++;
    }

    dst[n] = '\0';
}

static void unescape_newlines(char *s) {
    int r = 0;
    int w = 0;

    if (!s) return;

    while (s[r]) {
        if (s[r] == '\\' && s[r + 1] == 'n') {
            s[w++] = '\n';
            r += 2;
        } else {
            s[w++] = s[r++];
        }
    }

    s[w] = '\0';
}

static int parse_block_line(const char *line, ui_block_t *block, int index) {
    const char *p = line + 6;
    const char *sep1 = 0;
    const char *sep2 = 0;
    const char *sep3 = 0;

    for (int i = 0; p[i]; i++) {
        if (p[i] == '|') {
            sep1 = &p[i];
            break;
        }
    }

    if (!sep1) return 0;

    for (int i = 1; sep1[i]; i++) {
        if (sep1[i] == '|') {
            sep2 = &sep1[i];
            break;
        }
    }

    if (!sep2) return 0;

    for (int i = 1; sep2[i]; i++) {
        if (sep2[i] == '|') {
            sep3 = &sep2[i];
            break;
        }
    }

    char type_buf[24];
    char title_buf[96];
    char content_buf[256];
    char action_buf[160];

    copy_token(type_buf, sizeof(type_buf), p, sep1 - p);
    copy_token(title_buf, sizeof(title_buf), sep1 + 1, sep2 - sep1 - 1);

    if (sep3) {
        copy_token(content_buf, sizeof(content_buf), sep2 + 1, sep3 - sep2 - 1);
        copy_token(action_buf, sizeof(action_buf), sep3 + 1, 159);
    } else {
        copy_token(content_buf, sizeof(content_buf), sep2 + 1, 255);
        action_buf[0] = '\0';
    }

    int type = block_type_from_text(type_buf);
    if (!type) return 0;

    unescape_newlines(content_buf);

    int x, y, w, h;
    block_grid_position(index, &x, &y, &w, &h);

    block->type = type;
    block->x = x;
    block->y = y;
    block->w = w;
    block->h = h;
    block->title = ws_strdup_bounded(title_buf, 95);
    block->content = ws_strdup_bounded(content_buf, 255);
    block->action = action_buf[0] ? ws_strdup_bounded(action_buf, 159) : 0;
    block->flags = 0;

    if (!block->title) block->title = " Block ";
    if (!block->content) block->content = "";

    return 1;
}



static int ws_parse_weight(const char *line) {
    int last_pipe = -1;
    int value = 0;

    if (!line) return 1;

    for (int i = 0; line[i]; i++) {
        if (line[i] == '|') {
            last_pipe = i;
        }
    }

    if (last_pipe < 0) return 1;

    for (int i = last_pipe + 1; line[i]; i++) {
        if (line[i] < '0' || line[i] > '9') {
            return 1;
        }

        value = value * 10 + (line[i] - '0');
    }

    if (value < 1) value = 1;
    if (value > 9) value = 9;

    return value;
}

static int ws_add_node(workspace_node_t *nodes, int *node_count, int parent, int kind, int block_index, int weight) {
    if (*node_count >= WS_MAX_NODES) return -1;

    int id = *node_count;
    (*node_count)++;

    nodes[id].kind = kind;
    nodes[id].parent = parent;
    nodes[id].first_child = -1;
    nodes[id].last_child = -1;
    nodes[id].next_sibling = -1;
    nodes[id].block_index = block_index;
    nodes[id].weight = weight < 1 ? 1 : weight;

    if (parent >= 0) {
        if (nodes[parent].first_child < 0) {
            nodes[parent].first_child = id;
            nodes[parent].last_child = id;
        } else {
            nodes[nodes[parent].last_child].next_sibling = id;
            nodes[parent].last_child = id;
        }
    }

    return id;
}


static int ws_child_weight_sum(workspace_node_t *nodes, int node_id) {
    int sum = 0;
    int child = nodes[node_id].first_child;

    while (child >= 0) {
        int w = nodes[child].weight;
        if (w < 1) w = 1;

        sum += w;
        child = nodes[child].next_sibling;
    }

    if (sum < 1) sum = 1;
    return sum;
}

static int ws_child_count(workspace_node_t *nodes, int node_id) {
    int count = 0;
    int child = nodes[node_id].first_child;

    while (child >= 0) {
        count++;
        child = nodes[child].next_sibling;
    }

    return count;
}

static void ws_layout_node(workspace_node_t *nodes, int node_id, ui_block_t *blocks, int x, int y, int w, int h) {
    if (node_id < 0) return;

    workspace_node_t *node = &nodes[node_id];

    if (node->kind == WS_NODE_BLOCK) {
        int bi = node->block_index;

        if (bi >= 0) {
            blocks[bi].x = x;
            blocks[bi].y = y;
            blocks[bi].w = w;
            blocks[bi].h = h;
        }

        return;
    }

    int children = ws_child_count(nodes, node_id);
    if (children <= 0) return;

    int total_weight = ws_child_weight_sum(nodes, node_id);
    int child = node->first_child;
    int index = 0;

    if (node->kind == WS_NODE_ROW) {
        int used_w = 0;

        while (child >= 0) {
            int weight = nodes[child].weight;
            if (weight < 1) weight = 1;

            int cw = (w * weight) / total_weight;

            if (index == children - 1) {
                cw = w - used_w;
            }

            if (cw < 8) cw = 8;

            ws_layout_node(nodes, child, blocks, x + used_w, y, cw, h);

            used_w += cw;
            index++;
            child = nodes[child].next_sibling;
        }

        return;
    }

    /* root and column are vertical containers */
    int used_h = 0;

    while (child >= 0) {
        int weight = nodes[child].weight;
        if (weight < 1) weight = 1;

        int ch = (h * weight) / total_weight;

        if (index == children - 1) {
            ch = h - used_h;
        }

        if (ch < 4) ch = 4;

        ws_layout_node(nodes, child, blocks, x, y + used_h, w, ch);

        used_h += ch;
        index++;
        child = nodes[child].next_sibling;
    }
}

static void ws_apply_tree_layout(workspace_node_t *nodes, int root_id, ui_block_t *blocks) {
    if (root_id < 0) return;

    /*
     * Workspace content area:
     * y=0 title line
     * y=24 footer line
     * blocks live inside x=1..78, y=2..22
     */
    ws_layout_node(nodes, root_id, blocks, 1, 2, 78, 21);
}

static int parse_workspace_document(const char *content, char **title, ui_block_t *blocks, int *block_count) {
    int pos = 0;
    int count = 0;
    int tree_mode = 0;

    workspace_node_t nodes[WS_MAX_NODES];
    int node_count = 0;
    int stack[16];
    int stack_top = -1;
    int root_id = -1;

    *title = ws_strdup_bounded("Workspace", 64);
    *block_count = 0;

    if (!content || !starts_with(content, "WORKSPACE")) {
        return 0;
    }

    while (content[pos]) {
        char line[384];
        int li = 0;

        while (content[pos] && content[pos] != '\n' && li < 383) {
            if (content[pos] != '\r') {
                line[li++] = content[pos];
            }
            pos++;
        }

        line[li] = '\0';

        if (content[pos] == '\n') {
            pos++;
        }

        if (starts_with(line, "TITLE=")) {
            char *t = ws_strdup_bounded(line + 6, 80);
            if (t) *title = t;
        } else if (starts_with(line, "NODE=root")) {
            tree_mode = 1;

            root_id = ws_add_node(nodes, &node_count, -1, WS_NODE_ROOT, -1, 1);
            if (root_id < 0) return 0;

            stack_top = 0;
            stack[stack_top] = root_id;
        } else if (starts_with(line, "NODE=row")) {
            tree_mode = 1;

            if (stack_top < 0) {
                root_id = ws_add_node(nodes, &node_count, -1, WS_NODE_ROOT, -1, 1);
                if (root_id < 0) return 0;
                stack_top = 0;
                stack[stack_top] = root_id;
            }

            int parent = stack[stack_top];
            int id = ws_add_node(nodes, &node_count, parent, WS_NODE_ROW, -1, ws_parse_weight(line));
            if (id < 0) return 0;

            if (stack_top < 15) {
                stack_top++;
                stack[stack_top] = id;
            }
        } else if (starts_with(line, "NODE=column")) {
            tree_mode = 1;

            if (stack_top < 0) {
                root_id = ws_add_node(nodes, &node_count, -1, WS_NODE_ROOT, -1, 1);
                if (root_id < 0) return 0;
                stack_top = 0;
                stack[stack_top] = root_id;
            }

            int parent = stack[stack_top];
            int id = ws_add_node(nodes, &node_count, parent, WS_NODE_COLUMN, -1, ws_parse_weight(line));
            if (id < 0) return 0;

            if (stack_top < 15) {
                stack_top++;
                stack[stack_top] = id;
            }
        } else if (starts_with(line, "END")) {
            if (stack_top > 0) {
                stack_top--;
            }
        } else if (starts_with(line, "BLOCK=")) {
            if (count < WS_MAX_BLOCKS) {
                if (parse_block_line(line, &blocks[count], count)) {
                    if (tree_mode) {
                        if (stack_top < 0) {
                            root_id = ws_add_node(nodes, &node_count, -1, WS_NODE_ROOT, -1, 1);
                            if (root_id < 0) return 0;
                            stack_top = 0;
                            stack[stack_top] = root_id;
                        }

                        ws_add_node(nodes, &node_count, stack[stack_top], WS_NODE_BLOCK, count, 1);
                    }

                    count++;
                }
            }
        }
    }

    if (tree_mode && root_id >= 0) {
        ws_apply_tree_layout(nodes, root_id, blocks);
    }

    *block_count = count;
    return 1;
}


static void create_script_file(const char *name, const char *content) {
    if (!scripts_dir || !name || !content) return;

    vfs_node_t *file = vfs_find_child(scripts_dir, name);
    if (!file) file = vfs_create_file(scripts_dir, name);
    if (!file) return;

    vfs_write_file(file, content);
}

static void create_workspace_file(const char *name, const char *kind) {
    vfs_node_t *file = vfs_find_child(workspaces_dir, name);
    if (!file) file = vfs_create_file(workspaces_dir, name);
    if (!file) return;

    if (strcmp(kind, "coding") == 0) {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=AVEX Debug Workspace\n"
            "BLOCK=status|Build Status|Build failed\\nMissing dependency\n"
            "BLOCK=code|Code|app.ts:148\\nimport missing\\n\\nSuggested patch\n"
            "BLOCK=ai|AI Analysis|Cause detected\\nApply patch?\\nRun tests\n"
            "BLOCK=terminal|Terminal|npm run build\\nerror: module not found\n"
            "BLOCK=logs|Logs|api exited\\ncontainer running\n"
            "BLOCK=button|Action|Apply Fix|fs.touch patch.diff\n"
        );
    } else if (strcmp(kind, "server") == 0) {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=Server Ops Workspace\n"
            "BLOCK=status|Server|online\\nCPU OK\\nRAM OK\n"
            "BLOCK=terminal|SSH|ssh srv-admin\\nsystemctl status\n"
            "BLOCK=logs|Logs|nginx ok\\ndocker ok\\nfirewall ok\n"
            "BLOCK=ai|AI Ops|No critical issues\\nSuggested: update packages\n"
            "BLOCK=button|Action|Restart|workspace.open server.workspace\n"
            "BLOCK=text|Notes|Server workspace\\nservice-based UI\n"
        );
    } else if (strcmp(kind, "video") == 0) {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=Video Workspace\n"
            "BLOCK=status|Project|video loaded\\nFPS: 60\\nDuration: 2:35\n"
            "BLOCK=text|Preview|Preview service stub\\n[ frame output ]\n"
            "BLOCK=ai|AI Suggestions|Cut silence\\nNormalize audio\\nExport MP4\n"
            "BLOCK=code|Timeline|00:00 intro\\n00:30 scene\\n01:20 music\n"
            "BLOCK=logs|Audio|peak -3db\\nnoise low\n"
            "BLOCK=button|Export|Render|fs.touch render.mp4\n"
        );
    } else if (strcmp(kind, "tree") == 0) {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=Recursive Tree Workspace\n"
            "NODE=root|vertical\n"
            "NODE=row|horizontal\n"
            "BLOCK=status|Status|Recursive layout active\n"
            "BLOCK=code|Code|root > row > blocks\n"
            "BLOCK=ai|AI|Tree controls geometry\n"
            "END\n"
            "NODE=row|horizontal\n"
            "BLOCK=terminal|Terminal|make clean && make run\n"
            "NODE=column|vertical\n"
            "BLOCK=logs|Logs|Nested column node\n"
            "BLOCK=button|Action|Create Log|fs.touch recursive-tree.log\n"
            "END\n"
            "END\n"
            "END\n"
        );
    } else {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=Custom Workspace\n"
            "BLOCK=status|Status|Ready\n"
            "BLOCK=text|Notes|Edit this workspace file\n"
            "BLOCK=ai|AI|Intent analysis stub\n"
        );
    }
}

int workspace_builder_open(const char *name);

static int workspace_builder_open_file(const char *name) {
    return workspace_builder_open(name);
}

void workspace_builder_initialize(void) {
    vfs_node_t *root = vfs_get_root();
    workspaces_dir = ensure_dir(root, "workspaces");
    scripts_dir = ensure_dir(root, "scripts");

    create_script_file("create-lab.los",
        "wstitle generated.workspace Generated Tree\n"
        "wsnode generated.workspace root vertical\n"
        "wsnode generated.workspace row horizontal\n"
        "wsadd generated.workspace status Status Ready\n"
        "wsadd generated.workspace code Code generated-by-script\n"
        "wsadd generated.workspace ai AI Script-runner-works\n"
        "wsend generated.workspace\n"
        "wsnode generated.workspace row horizontal\n"
        "wsadd generated.workspace terminal Terminal make-run\n"
        "wsnode generated.workspace column vertical 2\n"
        "wsadd generated.workspace logs Logs nested-column\n"
        "wsbutton generated.workspace Action Touch fs.touch generated-tree.log\n"
        "wsend generated.workspace\n"
        "wsend generated.workspace\n"
        "open generated.workspace\n"
    );

    create_workspace_file("coding.workspace", "coding");
    create_workspace_file("server.workspace", "server");
    create_workspace_file("video.workspace", "video");
    create_workspace_file("tree.workspace", "tree");
    create_workspace_file("lab.workspace", "lab");

    fileassoc_register("workspace", workspace_builder_open_file);
    eventlog_add("workspace builder initialized");
}

void workspace_builder_list(void) {
    kprintf("Workspaces in /workspaces:\n");

    if (!workspaces_dir || !workspaces_dir->children) {
        kprintf("  none\n");
        return;
    }

    vfs_node_t *child = workspaces_dir->children;
    while (child) {
        if (child->type == VFS_FILE) {
            kprintf("  %s\n", child->name);
        }
        child = child->next;
    }
}

int workspace_builder_open(const char *name) {
    if (!workspaces_dir) return 0;

    vfs_node_t *file = vfs_find_child(workspaces_dir, name);
    if (!file || !file->content) return 0;

    ws_pool_reset();

    char *workspace_title = 0;
    ui_block_t blocks[WS_MAX_BLOCKS];
    int block_count = 0;

    if (!parse_workspace_document(file->content, &workspace_title, blocks, &block_count)) {
        kprintf("Invalid workspace document: %s\n", name);
        return 0;
    }

    if (block_count <= 0) {
        kprintf("Workspace has no valid blocks: %s\n", name);
        return 0;
    }

    layout_open_blocks(workspace_title, blocks, block_count);
    return 1;
}


int workspace_builder_template(const char *kind, const char *name) {
    if (!workspaces_dir || !kind || !name || !kind[0] || !name[0]) {
        return 0;
    }

    name = workspace_name_only(name);

    vfs_node_t *file = vfs_find_child(workspaces_dir, name);
    if (!file) {
        file = vfs_create_file(workspaces_dir, name);
    }

    if (!file) {
        return 0;
    }

    if (strcmp(kind, "coding") == 0) {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=Coding Workspace\n"
            "NODE=root|vertical|1\n"
            "NODE=row|horizontal|3\n"
            "BLOCK=list|Project Files|/projects\\n/src\\n/scripts\\n/docs\n"
            "BLOCK=code|Editor|main.c\\n// open a file from Project Files\\n// editor block stub\n"
            "BLOCK=ai|AI Assistant|Explain error\\nGenerate patch\\nPrepare commit\n"
            "END\n"
            "NODE=row|horizontal|2\n"
            "BLOCK=terminal|Terminal|make clean && make run\\ngit status\\n\n"
            "BLOCK=logs|Build Logs|No build started\\nWaiting for command\n"
            "BLOCK=button|Run Build|Build|shell:run /scripts/build.los\n"
            "END\n"
            "END\n"
        );

        eventlog_add("coding workspace template created");
        return 1;
    }

    if (strcmp(kind, "system") == 0) {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=System Workspace\n"
            "NODE=root|vertical|1\n"
            "NODE=row|horizontal|3\n"
            "BLOCK=status|Kernel|LOS kernel online\\nMode: i386\\nVFS: RAM\n"
            "BLOCK=status|Memory|Heap active\\nPaging enabled\\nPMM ready\n"
            "BLOCK=status|Services|AI runtime ready\\nService bus ready\\nIntent engine ready\n"
            "END\n"
            "NODE=row|horizontal|2\n"
            "BLOCK=logs|Event Log|Use dmesg for details\\nSystem initialized\n"
            "BLOCK=button|Actions|Create Health Report|shell:echo System OK > /notes/health.report\n"
            "END\n"
            "END\n"
        );

        eventlog_add("system workspace template created");
        return 1;
    }

    if (strcmp(kind, "notes") == 0) {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=Notes Workspace\n"
            "NODE=root|vertical|1\n"
            "NODE=row|horizontal|2\n"
            "BLOCK=list|Notes|/notes/welcome.txt\\n/notes/todo.txt\\n/notes/ideas.txt\n"
            "BLOCK=text|Current Note|Write notes with echo or nano\\nExample:\\nnano /notes/todo.txt\n"
            "END\n"
            "NODE=row|horizontal|2\n"
            "BLOCK=ai|AI Summary|Summarize notes\\nFind todos\\nCreate plan\n"
            "BLOCK=button|Actions|Create todo.txt|shell:echo TODO > /notes/todo.txt\n"
            "END\n"
            "END\n"
        );

        eventlog_add("notes workspace template created");
        return 1;
    }

    if (strcmp(kind, "services") == 0) {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=Services Workspace\n"
            "BLOCK=status|Service Bus|service bus online\\nworkspace service ready\n"
            "BLOCK=status|AI Runtime|ai runtime initialized\\nintent engine initialized\n"
            "BLOCK=status|Apps|editor\\nnorton\\nworkspace layout\n"
            "BLOCK=terminal|Service Calls|service workspace.list none\\nservice model.status none\n"
            "BLOCK=logs|Events|Use dmesg to inspect event log\n"
            "BLOCK=button|Action|Create service check|shell:echo Services OK > /notes/service.check\n"
        );

        eventlog_add("services workspace template created");
        return 1;
    }

    if (strcmp(kind, "debug") == 0) {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=Debug Build Workspace\n"
            "NODE=root|vertical|1\n"
            "NODE=row|horizontal|3\n"
            "BLOCK=status|Problem|Build error detected\nInspect compiler output\nCheck recent patch\n"
            "BLOCK=code|Likely Files|kernel/shell/shell.c\nkernel/intent/intent.c\nkernel/workspace/workspace_builder.c\n"
            "BLOCK=ai|AI Debug Plan|1. Read error\n2. Locate symbol\n3. Patch smallest surface\n4. Rebuild\n"
            "END\n"
            "NODE=row|horizontal|2\n"
            "BLOCK=terminal|Build Commands|make clean\nmake run\ngit status\n"
            "BLOCK=logs|Build Log|No captured log yet\nUse /notes/build.log for script output\n"
            "BLOCK=button|Actions|Run Build|shell:run /scripts/build.los\n"
            "END\n"
            "END\n"
        );

        eventlog_add("debug task workspace generated");
        return 1;
    }

    if (strcmp(kind, "overview") == 0) {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=System Overview Workspace\n"
            "NODE=root|vertical|1\n"
            "NODE=row|horizontal|3\n"
            "BLOCK=status|Kernel|LOS online\nFreestanding i386 kernel\nGRUB/QEMU boot\n"
            "BLOCK=status|Subsystems|VFS\nShell\nServices\nIntent\nWorkspace UI\n"
            "BLOCK=ai|System Summary|Show current architecture\nSuggest next subsystem\nPrepare roadmap\n"
            "END\n"
            "NODE=row|horizontal|2\n"
            "BLOCK=logs|Diagnostics|Use dmesg\nUse services\nUse workstatus\n"
            "BLOCK=button|Actions|Show Services|shell:services\n"
            "END\n"
            "END\n"
        );

        eventlog_add("overview task workspace generated");
        return 1;
    }

    if (strcmp(kind, "writing") == 0) {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=Writing / Notes Workspace\n"
            "NODE=root|vertical|1\n"
            "NODE=row|horizontal|2\n"
            "BLOCK=list|Notes|/notes/welcome.txt\n/notes/todo.txt\n/notes/ideas.txt\n/notes/draft.txt\n"
            "BLOCK=text|Draft|Use nano /notes/draft.txt\nOr create notes from buttons\n"
            "END\n"
            "NODE=row|horizontal|2\n"
            "BLOCK=ai|Writing Assistant|Summarize\nRewrite\nExtract todos\nPlan next steps\n"
            "BLOCK=button|Actions|Create Draft|shell:echo Draft started > /notes/draft.txt\n"
            "END\n"
            "END\n"
        );

        eventlog_add("writing task workspace generated");
        return 1;
    }

    if (strcmp(kind, "planning") == 0) {
        vfs_write_file(file,
            "WORKSPACE\n"
            "TITLE=Project Planning Workspace\n"
            "NODE=root|vertical|1\n"
            "NODE=row|horizontal|3\n"
            "BLOCK=status|Goal|Define task\nSplit into milestones\nTrack progress\n"
            "BLOCK=list|Milestones|v20.20 generated workspaces\nv20.21 command registry\nv20.22 task files\n"
            "BLOCK=ai|Planner|Find next smallest patch\nAvoid big rewrites\nKeep build green\n"
            "END\n"
            "NODE=row|horizontal|2\n"
            "BLOCK=text|Notes|Use /notes/plan.txt\nUse startup.los for boot tasks\n"
            "BLOCK=button|Actions|Create Plan File|shell:echo LOS plan > /notes/plan.txt\n"
            "END\n"
            "END\n"
        );

        eventlog_add("planning task workspace generated");
        return 1;
    }

    vfs_write_file(file,
        "WORKSPACE\n"
        "TITLE=Custom Template Workspace\n"
        "BLOCK=status|Status|Ready\n"
        "BLOCK=text|Notes|Unknown template kind\\nEdit this workspace file\n"
        "BLOCK=ai|AI|Template generation stub\n"
    );

    eventlog_add("custom workspace template created");
    return 1;
}


int workspace_builder_create(const char *name, const char *kind) {
    if (!workspaces_dir) return 0;
    create_workspace_file(name, kind);
    eventlog_add("workspace file created");
    return 1;
}

static int ws_append_line(vfs_node_t *file, const char *line) {
    static char buffer[2048];

    if (!file || !line) return 0;

    int pos = 0;

    if (file->content) {
        for (int i = 0; file->content[i] && pos < 1900; i++) {
            buffer[pos++] = file->content[i];
        }
    }

    if (pos > 0 && buffer[pos - 1] != '\n' && pos < 1900) {
        buffer[pos++] = '\n';
    }

    for (int i = 0; line[i] && pos < 2000; i++) {
        buffer[pos++] = line[i];
    }

    if (pos < 2047) {
        buffer[pos++] = '\n';
    }

    buffer[pos] = '\0';

    return vfs_write_file(file, buffer);
}

int workspace_builder_set_title(const char *name, const char *title) {
    static char buffer[256];

    if (!workspaces_dir || !name || !title) return 0;

    vfs_node_t *file = vfs_find_child(workspaces_dir, name);
    if (!file) file = vfs_create_file(workspaces_dir, name);
    if (!file) return 0;

    int pos = 0;
    const char *prefix = "WORKSPACE\nTITLE=";

    for (int i = 0; prefix[i] && pos < 240; i++) {
        buffer[pos++] = prefix[i];
    }

    for (int i = 0; title[i] && pos < 240; i++) {
        buffer[pos++] = title[i];
    }

    buffer[pos++] = '\n';
    buffer[pos] = '\0';

    return vfs_write_file(file, buffer);
}

int workspace_builder_add_block(const char *name, const char *type, const char *title, const char *content) {
    static char line[384];

    if (!workspaces_dir || !name || !type || !title || !content) return 0;

    vfs_node_t *file = vfs_find_child(workspaces_dir, name);
    if (!file) file = vfs_create_file(workspaces_dir, name);
    if (!file) return 0;

    if (!file->content || file->size == 0) {
        vfs_write_file(file, "WORKSPACE\nTITLE=Untitled Workspace\n");
    }

    int pos = 0;
    const char *prefix = "BLOCK=";

    for (int i = 0; prefix[i] && pos < 360; i++) line[pos++] = prefix[i];
    for (int i = 0; type[i] && pos < 360; i++) line[pos++] = type[i];

    if (pos < 360) line[pos++] = '|';

    for (int i = 0; title[i] && pos < 360; i++) line[pos++] = title[i];

    if (pos < 360) line[pos++] = '|';

    for (int i = 0; content[i] && pos < 360; i++) {
        if (content[i] == '|') {
            line[pos++] = '/';
        } else {
            line[pos++] = content[i];
        }
    }

    line[pos] = '\0';

    return ws_append_line(file, line);
}

int workspace_builder_add_button(const char *name, const char *title, const char *label, const char *action) {
    static char line[448];

    if (!workspaces_dir || !name || !title || !label || !action) return 0;

    vfs_node_t *file = vfs_find_child(workspaces_dir, name);
    if (!file) file = vfs_create_file(workspaces_dir, name);
    if (!file) return 0;

    if (!file->content || file->size == 0) {
        vfs_write_file(file, "WORKSPACE\nTITLE=Untitled Workspace\n");
    }

    int pos = 0;
    const char *prefix = "BLOCK=button|";

    for (int i = 0; prefix[i] && pos < 430; i++) line[pos++] = prefix[i];
    for (int i = 0; title[i] && pos < 430; i++) line[pos++] = title[i];

    if (pos < 430) line[pos++] = '|';

    for (int i = 0; label[i] && pos < 430; i++) line[pos++] = label[i];

    if (pos < 430) line[pos++] = '|';

    for (int i = 0; action[i] && pos < 430; i++) {
        if (action[i] == '|') {
            line[pos++] = '/';
        } else {
            line[pos++] = action[i];
        }
    }

    line[pos] = '\0';

    return ws_append_line(file, line);
}


int workspace_builder_add_node(const char *name, const char *kind, const char *orientation, const char *weight) {
    static char line[128];

    if (!workspaces_dir || !name || !kind || !orientation) return 0;

    name = workspace_name_only(name);

    vfs_node_t *file = vfs_find_child(workspaces_dir, name);
    if (!file) file = vfs_create_file(workspaces_dir, name);
    if (!file) return 0;

    if (!file->content || !file->content[0]) {
        vfs_write_file(file, "WORKSPACE\nTITLE=Tree Workspace\n");
    }

    int pos = 0;
    const char *prefix = "NODE=";

    for (int i = 0; prefix[i] && pos < 120; i++) line[pos++] = prefix[i];
    for (int i = 0; kind[i] && pos < 120; i++) line[pos++] = kind[i];

    if (pos < 120) line[pos++] = '|';
    for (int i = 0; orientation[i] && pos < 120; i++) line[pos++] = orientation[i];

    if (weight && weight[0]) {
        if (pos < 120) line[pos++] = '|';
        for (int i = 0; weight[i] && pos < 120; i++) line[pos++] = weight[i];
    }

    line[pos] = '\0';

    return ws_append_line(file, line);
}

int workspace_builder_end_node(const char *name) {
    if (!workspaces_dir || !name) return 0;

    name = workspace_name_only(name);

    vfs_node_t *file = vfs_find_child(workspaces_dir, name);
    if (!file) return 0;

    return ws_append_line(file, "END");
}
