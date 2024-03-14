#pragma once

#define VIDEO_VBE 0x00
#define VIDEO_GOP 0x01

#include <drivers/impl.h>

#include <kernel/include/C/typedefs.h>

struct kern_video
{
    uint8_t mode;           // The target video mode.
    void *info_structure;   // The pointer to the info structure for current video mode.
};

void video_init();
void video_clear();

int video_width();
int video_height();
int video_bpp();

void *video_fb();
size_t video_fb_size();

void video_setmode(uint8_t mode, uint32_t width, uint32_t height);
