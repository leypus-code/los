#ifndef LOS_PAGER_H
#define LOS_PAGER_H

void pager_open(const char *title, const char **lines, int count);
int pager_active(void);
void pager_key(int key);

#endif
