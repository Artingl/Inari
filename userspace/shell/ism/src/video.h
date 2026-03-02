#ifndef _INARI_ISM_VIDEO_H
#define _INARI_ISM_VIDEO_H

#include <stdint.h>
#include <stddef.h>

#include <list.h>

struct ism_video_map {
    uint8_t *base;
    size_t size;
};

int ism_video_init(const char *device);
int ism_video_cleanup(void);
int ism_video_resolution(int32_t *width, int32_t *height, uint32_t *bpp);
int ism_video_flush(void);
int ism_video_map(struct ism_video_map *size);
int ism_video_blit(uint8_t *buf, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t px_len);
int ism_video_fill_rect(uint32_t color, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t px_len);

#endif