#ifndef _INARI_ISM_WINDOW_H
#define _INARI_ISM_WINDOW_H

#define ISM_MAX_WINDOW_NAME_LN   256
#define ISM_WINDOW_ROOT          0
#define ISM_WINDOW_ROOT_BGCOLOR  0x018281

#define ISM_WINDOW_FLAG_FRAME    (1 << 0)
#define ISM_WINDOW_FLAG_BORDER   (1 << 1)

#include <list.h>

#include "core.h"
#include "video.h"

#define ISM_CURSOR_NORMAL       0
#define ISM_CURSOR_HAND         1
#define ISM_CURSOR_BEAM         2
#define ISM_CURSOR_HOURGLASS    3

typedef uint32_t wind_t;    // window id

struct ism_window_event {
    uint8_t type;
};

struct ism_buffer {
    int32_t width, height;
    uint8_t *base;
    size_t size;
};

struct ism_window {
    int32_t x, y;
    int32_t width, height;
    uint32_t bg_color;
    uint32_t flags;
    wind_t window_id;

    char name[ISM_MAX_WINDOW_NAME_LN];

    uint8_t controls_state[3];   // 0 - close; 1 - maximize; 2 - minimize

    struct ism_buffer buffer;
    struct ism_window *parent;

    struct list_head children;
    struct list_head list;
};

int ism_window_init(void);
int ism_window_cleanup(void);
int ism_window_render(void);
int ism_window_update(void);
int ism_window_create(wind_t *id, wind_t parent, const char *name, uint32_t flags, int32_t x, int32_t y, int32_t width, int32_t height);
int ism_window_destroy(wind_t id);

#endif