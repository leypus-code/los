#ifndef LOS_NORTON_H
#define LOS_NORTON_H

#include "vfs.h"

void norton_set_start_dir(vfs_node_t *dir);
void norton_enter(void);
void norton_resume(void);
void norton_force_clean_exit(void);
void norton_handle_key(int key);

#endif
