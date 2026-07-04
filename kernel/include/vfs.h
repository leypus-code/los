#ifndef LOS_VFS_H
#define LOS_VFS_H

#include <stdint.h>

typedef enum {
    VFS_DIRECTORY = 1,
    VFS_FILE = 2
} vfs_node_type_t;

typedef struct vfs_node {
    char name[32];
    vfs_node_type_t type;
    char *content;
    uint32_t size;
    struct vfs_node *parent;
    struct vfs_node *children;
    struct vfs_node *next;
} vfs_node_t;

void vfs_initialize(void);
vfs_node_t *vfs_get_root(void);
vfs_node_t *vfs_mkdir(vfs_node_t *parent, const char *name);
vfs_node_t *vfs_create_file(vfs_node_t *parent, const char *name);
vfs_node_t *vfs_find_child(vfs_node_t *parent, const char *name);
vfs_node_t *vfs_resolve(vfs_node_t *cwd, const char *path);
int vfs_write_file(vfs_node_t *file, const char *content);
void vfs_list(vfs_node_t *node);
void vfs_tree(vfs_node_t *node, int depth);
void vfs_path(vfs_node_t *node, char *buffer, int max);
int vfs_delete(vfs_node_t *node);
int vfs_rename(vfs_node_t *node, const char *new_name);

#endif
