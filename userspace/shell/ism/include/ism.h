#ifndef _INARI_ISM_API_H
#define _INARI_ISM_API_H

#include <types.h>

typedef int32_t wind_t;

#define ISM_IPC_CREATE_WINDOW   0x00
#define ISM_IPC_DRAW_WINDOW     0x01

#define ISM_MAX_WINDOW_NAME_LN   256
#define ISM_WINDOW_ROOT          0
#define ISM_WINDOW_ROOT_BGCOLOR  0x018281

#define ISM_WINDOW_FLAG_FRAME    (1 << 0)
#define ISM_WINDOW_FLAG_BORDER   (1 << 1)

#define ISM_CURSOR_NORMAL       0
#define ISM_CURSOR_HAND         1
#define ISM_CURSOR_BEAM         2
#define ISM_CURSOR_HOURGLASS    3

struct ism_create_window {
    char name[ISM_MAX_WINDOW_NAME_LN];

    wind_t parent;
    uint32_t flags;

    int32_t x, y;
    int32_t width, height;
} __attribute__((packed));

#endif
