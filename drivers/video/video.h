#pragma once

#define VIDEO_VGA_TEXT 0x00
#define VIDEO_VBE_TEXT 0x01

#define TEXT_FONT_WIDTH 8
#define TEXT_FONT_WIDTH 8

#include <drivers/impl.h>

#include <kernel/include/C/typedefs.h>

struct kern_video
{
    uint8_t mode;           // The target video mode.
    void *info_structure;   // The pointer to the info structure for current video mode.
};

void video_init();

void video_clear();
void video_text_render(uint32_t *buffer, size_t width, size_t height);

int video_text_print_at(uint32_t *data, uint8_t color, size_t offset, size_t length);
int video_text_set_at(char c, uint8_t color, size_t offset);

int video_text_width();
int video_text_height();
int video_width();
int video_height();
int video_bpp();

void *video_fb();
size_t video_fb_size();

void video_setmode(uint8_t mode, uint32_t width, uint32_t height);
