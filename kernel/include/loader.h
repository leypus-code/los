#ifndef LOS_LOADER_H
#define LOS_LOADER_H

#include "vfs.h"

void loader_initialize(void);
int loader_load_lap(const char *name);
int loader_load_lap_from_vfs(vfs_node_t *cwd, const char *name);
void loader_list_formats(void);

#endif
