#include "../include/vfs.h"
#include "../include/memory.h"
#include "../include/kprintf.h"
#include "../include/theme.h"
#include "../include/string.h"

static vfs_node_t *root = 0;

int vfs_streq(const char *a, const char *b);

static void copy_name(char *dst, const char *src) {
    uint32_t i = 0;

    while (src[i] && i < 31) {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
}

static vfs_node_t *vfs_create_node(const char *name, vfs_node_type_t type) {
    vfs_node_t *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));

    if (!node) {
        return 0;
    }

    copy_name(node->name, name);
    node->type = type;
    node->content = 0;
    node->size = 0;
    node->parent = 0;
    node->children = 0;
    node->next = 0;

    return node;
}

void vfs_initialize(void) {
    root = vfs_create_node("/", VFS_DIRECTORY);

    vfs_mkdir(root, "system");
    vfs_mkdir(root, "users");
    vfs_mkdir(root, "models");
    vfs_mkdir(root, "apps");
    vfs_mkdir(root, "tmp");
    vfs_mkdir(root, "dev");

    vfs_node_t *scripts = vfs_mkdir(root, "scripts");
    vfs_node_t *workspaces = vfs_mkdir(root, "workspaces");
    vfs_mkdir(root, "projects");
    vfs_mkdir(root, "notes");

    if (scripts) {
        vfs_node_t *startup = vfs_create_file(scripts, "startup.los");

        if (startup) {
            vfs_write_file(startup,
                "# LOS startup script\n"
                "theme terminal\n"
                "mkdir -p /projects/los\n"
                "mkdir -p /notes\n"
                "echo Welcome to LOS > /notes/welcome.txt\n"
            );
        }
    }

    if (workspaces) {
        vfs_node_t *readme = vfs_create_file(workspaces, "README.txt");

        if (readme) {
            vfs_write_file(readme,
                "LOS workspaces directory\n"
                "Use mkworkspace or workspace commands to create generated UI screens.\n"
            );
        }
    }
}

vfs_node_t *vfs_get_root(void) {
    return root;
}

vfs_node_t *vfs_mkdir(vfs_node_t *parent, const char *name) {
    if (!parent || !name || !name[0]) {
        return 0;
    }

    vfs_node_t *existing = vfs_find_child(parent, name);
    if (existing) {
        return existing;
    }

    vfs_node_t *node = vfs_create_node(name, VFS_DIRECTORY);
    if (!node) {
        return 0;
    }

    node->parent = parent;
    node->next = parent->children;
    parent->children = node;

    return node;
}



static int vfs_name_less(const char *a, const char *b) {
    int i = 0;

    if (!a) return 0;
    if (!b) return 1;

    while (a[i] && b[i]) {
        char ca = a[i];
        char cb = b[i];

        if (ca >= 'A' && ca <= 'Z') ca = ca + 32;
        if (cb >= 'A' && cb <= 'Z') cb = cb + 32;

        if (ca < cb) return 1;
        if (ca > cb) return 0;

        i++;
    }

    return a[i] == '\0' && b[i] != '\0';
}

static void vfs_print_list_node(vfs_node_t *child) {
    if (!child) return;

    if (child->type == VFS_DIRECTORY) {
        theme_set_dir_tag();
        kprintf("<DIR>  ");
        theme_set_dir_name();
        kprintf("%s\n", child->name);
    } else {
        theme_set_file_tag();
        kprintf("<FILE> ");
        theme_set_file_tag();
        kprintf("%s\n", child->name);
    }
}

void vfs_list(vfs_node_t *node) {
    if (!node || node->type != VFS_DIRECTORY) {
        theme_set_error();
        kprintf("Not a directory\n");
        theme_set_normal();
        return;
    }

    vfs_node_t *items[128];
    int count = 0;

    vfs_node_t *child = node->children;
    while (child && count < 128) {
        items[count++] = child;
        child = child->next;
    }

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            int swap = 0;

            if (items[i]->type != items[j]->type) {
                /* directories first, files second */
                if (items[i]->type != VFS_DIRECTORY && items[j]->type == VFS_DIRECTORY) {
                    swap = 1;
                }
            } else if (vfs_name_less(items[j]->name, items[i]->name)) {
                swap = 1;
            }

            if (swap) {
                vfs_node_t *tmp = items[i];
                items[i] = items[j];
                items[j] = tmp;
            }
        }
    }

    for (int i = 0; i < count; i++) {
        vfs_print_list_node(items[i]);
    }

    theme_set_normal();
}

static void print_indent(int depth) {
    for (int i = 0; i < depth; i++) {
        theme_set_tree_line();
        kprintf("|   ");
    }
}

void vfs_tree(vfs_node_t *node, int depth) {
    if (!node) {
        return;
    }

    if (depth == 0) {
        theme_set_root();
        kprintf("/\n");
    } else {
        print_indent(depth - 1);

        theme_set_tree_line();
        kprintf("|-- ");

        if (node->type == VFS_DIRECTORY) {
            theme_set_dir_name();
        } else {
            theme_set_normal();
        }

        kprintf("%s\n", node->name);
    }

    vfs_node_t *child = node->children;

    while (child) {
        vfs_tree(child, depth + 1);
        child = child->next;
    }

    theme_set_normal();
}

static int vfs_strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void vfs_strcpy(char *dst, const char *src) {
    int i = 0;
    while (src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void vfs_append(char *dst, const char *src, int max) {
    int d = vfs_strlen(dst);
    int i = 0;

    while (src[i] && d < max - 1) {
        dst[d++] = src[i++];
    }

    dst[d] = '\0';
}

void vfs_path(vfs_node_t *node, char *buffer, int max) {
    if (!node || !buffer || max <= 0) {
        return;
    }

    buffer[0] = '\0';

    if (node == root) {
        vfs_strcpy(buffer, "/");
        return;
    }

    const char *parts[16];
    int count = 0;

    vfs_node_t *current = node;

    while (current && current != root && count < 16) {
        parts[count++] = current->name;
        current = current->parent;
    }

    vfs_strcpy(buffer, "/");

    for (int i = count - 1; i >= 0; i--) {
        vfs_append(buffer, parts[i], max);
        if (i > 0) {
            vfs_append(buffer, "/", max);
        }
    }
}

int vfs_delete(vfs_node_t *node) {
    if (!node || !node->parent) {
        return 0;
    }

    if (node->children) {
        return 0;
    }

    vfs_node_t *parent = node->parent;
    vfs_node_t *current = parent->children;
    vfs_node_t *prev = 0;

    while (current) {
        if (current == node) {
            if (prev) {
                prev->next = current->next;
            } else {
                parent->children = current->next;
            }

            kfree(current);
            return 1;
        }

        prev = current;
        current = current->next;
    }

    return 0;
}

vfs_node_t *vfs_create_file(vfs_node_t *parent, const char *name) {
    if (!parent || !name || !name[0]) {
        return 0;
    }

    vfs_node_t *existing = vfs_find_child(parent, name);
    if (existing) {
        return existing;
    }

    vfs_node_t *node = vfs_create_node(name, VFS_FILE);
    if (!node) {
        return 0;
    }

    node->parent = parent;
    node->next = parent->children;
    parent->children = node;

    return node;
}


vfs_node_t *vfs_find_child(vfs_node_t *parent, const char *name) {
    if (!parent || parent->type != VFS_DIRECTORY) {
        return 0;
    }

    vfs_node_t *child = parent->children;

    while (child) {
        if (vfs_streq(name, child->name)) {
            return child;
        }

        child = child->next;
    }

    return 0;
}

int vfs_write_file(vfs_node_t *file, const char *content) {
    if (!file || file->type != VFS_FILE) {
        return 0;
    }

    uint32_t len = 0;
    while (content[len]) {
        len++;
    }

    char *buffer = (char *)kmalloc(len + 1);

    if (!buffer) {
        return 0;
    }

    for (uint32_t i = 0; i < len; i++) {
        buffer[i] = content[i];
    }

    buffer[len] = '\0';

    if (file->content) {
        kfree(file->content);
    }

    file->content = buffer;
    file->size = len;

    return 1;
}

int vfs_streq(const char *a, const char *b) {
    int i = 0;
    while (a[i] || b[i]) {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }
    return 1;
}

vfs_node_t *vfs_resolve(vfs_node_t *cwd, const char *path) {
    if (!path || !path[0]) {
        return cwd ? cwd : vfs_get_root();
    }

    vfs_node_t *node = 0;
    int i = 0;

    if (path[0] == '/') {
        node = vfs_get_root();
        i = 1;
    } else {
        node = cwd ? cwd : vfs_get_root();
    }

    while (path[i]) {
        while (path[i] == '/') {
            i++;
        }

        if (!path[i]) {
            break;
        }

        char part[32];
        int p = 0;

        while (path[i] && path[i] != '/') {
            if (p < 31) {
                part[p++] = path[i];
            }
            i++;
        }

        part[p] = '\0';

        if (p == 0) {
            continue;
        }

        if (strcmp(part, ".") == 0) {
            continue;
        }

        if (strcmp(part, "..") == 0) {
            if (node && node->parent) {
                node = node->parent;
            }
            continue;
        }

        if (!node || node->type != VFS_DIRECTORY) {
            return 0;
        }

        node = vfs_find_child(node, part);
        if (!node) {
            return 0;
        }
    }

    return node;
}


int vfs_rename(vfs_node_t *node, const char *new_name) {
    if (!node || !new_name || !new_name[0]) {
        return 0;
    }

    int i = 0;
    while (new_name[i] && i < 31) {
        node->name[i] = new_name[i];
        i++;
    }

    node->name[i] = '\0';
    return 1;
}
