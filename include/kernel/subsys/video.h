#ifndef _INARI_VIDEO_H
#define _INARI_VIDEO_H

#include <misc/types.h>

struct video_device
{
    uintptr_t base;
};

int video_init(void);
// int video_add_device(uintptr_t )

#endif