#ifndef LOS_PACKAGE_H
#define LOS_PACKAGE_H

#include "vfs.h"

void package_initialize(void);
int package_install(vfs_node_t *cwd, const char *lap_name);
int package_remove(const char *app_name);
void package_list(void);

#endif
