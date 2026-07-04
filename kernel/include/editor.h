#ifndef LOS_EDITOR_H
#define LOS_EDITOR_H

#include "vfs.h"

void editor_enter(void);
void editor_open_file(vfs_node_t *file);
void editor_handle_key(int key);

#endif
