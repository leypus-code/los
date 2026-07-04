#ifndef LOS_LAYOUT_H
#define LOS_LAYOUT_H

#include "uiblock.h"

typedef struct workspace {
    const char *title;
    ui_block_t blocks[24];
    int block_count;
} workspace_t;

void layout_initialize(void);
void layout_open_debug_workspace(void);
void layout_open_server_workspace(void);
void layout_open_video_workspace(void);
void layout_open_blocks(const char *workspace_title, ui_block_t *blocks, int count);
void layout_handle_key(int key);
void layout_status(void);

#endif
