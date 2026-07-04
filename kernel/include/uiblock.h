#ifndef LOS_UIBLOCK_H
#define LOS_UIBLOCK_H

#include <stdint.h>

#define UI_BLOCK_TEXT     1
#define UI_BLOCK_STATUS   2
#define UI_BLOCK_BUTTON   3
#define UI_BLOCK_TERMINAL 4
#define UI_BLOCK_CODE     5
#define UI_BLOCK_LOGS     6
#define UI_BLOCK_LIST     7
#define UI_BLOCK_AI       8

typedef struct ui_block {
    int type;
    int x;
    int y;
    int w;
    int h;
    const char *title;
    const char *content;
    const char *action;
    uint32_t flags;
} ui_block_t;

void uiblock_draw(ui_block_t *block, int active);

#endif
