#include <sys.h>
#include <io.h>
#include <errno.h>
#include <string.h>
#include <lib.h>
#include <list.h>

#include "video.h"
#include "input.h"
#include "core.h"

#define VIDEO_IOCTL_INFO           0 // Current mode info
#define VIDEO_IOCTL_MODE_SWITCH    1 // Switch to different mode
#define VIDEO_IOCTL_MODE_FIND_NEXT 2 // Iterate through all modes
#define VIDEO_IOCTL_BLIT           4 // Blit using video device
#define VIDEO_IOCTL_DISABLE        5 // Disable the video device (e.g. switch to text mode in VESA)
#define VIDEO_IOCTL_FILL_RECT      6 // Fill a rect with dimensions and static color
#define VIDEO_IOCTL_MAP            7 // Map video memory to process's virtual memory and provide info

struct video_mode_info;
struct video_blit;
struct video_fill_rect;
struct video_map_info;

struct video_fill_rect {
    uint8_t format;
    uint32_t color;
    int32_t x, y;
    int32_t width, height;
} __attribute__((packed));

struct video_map_info {
    uint8_t *base;
    size_t size;
} __attribute__((packed));

struct video_blit {
    uint8_t format;
    uint8_t *buffer;
    int32_t x, y;
    int32_t width, height;
} __attribute__((packed));

#define VIDEO_R8G8B8_FORMAT 0
#define VIDEO_GRAY8_FORMAT  1

struct video_mode_info {
    /* Mode info */
    int32_t width;
    int32_t height;
    uint32_t bpp;

    /* Video driver can set this value if this is the default mode for the display */
    uint8_t is_default;

    /* Driver specific mode id */
    uint32_t mode_id;
} __attribute__((packed));

static handle_t video_handle = 0;
static struct video_mode_info current_mode;
static struct video_map_info video_map;

int ism_video_init(const char *device)
{
    int res;
    struct video_mode_info info = {0}, preferred_mode;
    if (video_handle || !device)  return -1;

    /* Try to open the video device */
    if ((res = open(&video_handle, device, WRITE)) != 0) {
        printf("%s: Unable to open video device %s: %s.\n", get_name(), device, errno(res));
        return res;
    }

    /* Try to set the preferred display resolution */
    while (ioctl(video_handle, VIDEO_IOCTL_MODE_FIND_NEXT, &info) > 0) {
        /* Save any first mode if info struct is empty */
        if (info.width == 0) {
            memcpy((void*)&preferred_mode, (void*)&info, sizeof(struct video_mode_info));
        }

        /* Always preferr 32bpp */
        if (info.bpp == 32) {
            memcpy((void*)&preferred_mode, (void*)&info, sizeof(struct video_mode_info));
        }

        /* If found default mode in 32bpp, use this one */
        if (info.bpp == 32 && info.is_default) {
            memcpy((void*)&preferred_mode, (void*)&info, sizeof(struct video_mode_info));
            break;
        }
    }

    if (info.width == 0) {
        printf("%s: Unable to find any available video mode.\n", get_name());
        return ism_video_cleanup() - 1;
    }

    printf("%s: Setting mode: %dx%d_%d\n", get_name(), preferred_mode.width, preferred_mode.height, preferred_mode.bpp);

    if ((res = ioctl(video_handle, VIDEO_IOCTL_MODE_SWITCH, &preferred_mode)) != 0) {
        printf("%s: Unable to set the mode: %s.\n", get_name(), errno(res));
        return ism_video_cleanup() - 1;
    }

    /* Save current mode */
    memcpy((void*)&current_mode, (void*)&preferred_mode, sizeof(struct video_mode_info));

    /* Map video memory to process's local memory */
    if ((res = ioctl(video_handle, VIDEO_IOCTL_MAP, &video_map)) != 0) {
        printf("%s: Unable to map video memory: %s.\n", get_name(), errno(res));
        return ism_video_cleanup() - 1;
    }

    return 0;
}

int ism_video_map(struct ism_video_map *size)
{
    if (!video_handle)  return -1;
    if (size) {
        size->base = video_map.base;
        size->size = video_map.size;
    }
    return 0;
}

int ism_video_resolution(int32_t *width, int32_t *height, uint32_t *bpp)
{
    if (!video_handle)  return -1;
    if (width)  *width = current_mode.width;
    if (height) *height = current_mode.height;
    if (bpp)    *bpp = current_mode.bpp;
    return 0;
}

int ism_video_cleanup(void)
{
    if (!video_handle)  return -1;

    /* Disable video */
    ioctl(video_handle, VIDEO_IOCTL_DISABLE, NULL);
    close(video_handle);
    video_handle = 0;

    return 0;
}

#pragma GCC push_options
#pragma GCC optimize("O3")

int ism_video_flush(void)
{
    return 0;
}

int ism_video_blit(uint8_t *buf, int32_t x_orig, int32_t y_orig, int32_t width, int32_t height, uint32_t px_len)
{
    if (!video_handle)  return -1;

    int32_t x, y;
    uint8_t r, g, b;
    uintptr_t off, buf_off = 0, buf_size = width * height * px_len;

    for (y = y_orig; y < height + y_orig; y++)
        for (x = x_orig; x < width + x_orig; x++) {
            if (y >= current_mode.height || x >= current_mode.width || x < 0 || y < 0)
                continue;
            off = y * current_mode.width + x;
            buf_off = (y - y_orig) * px_len * width + (x - x_orig) * px_len;
            if (off >= video_map.size || buf_off + 2 >= buf_size)
                continue;
            r = buf[buf_off + 0];
            g = buf[buf_off + (px_len > 1 ? 1 : 0)];
            b = buf[buf_off + (px_len > 2 ? 2 : 0)];

            ((uint32_t*)video_map.base)[off] = (((((0xff << 8) | r) << 8) | g) << 8) | b;
        }

    return 0;
}

int ism_video_fill_rect(uint32_t color, int32_t x_orig, int32_t y_orig, int32_t width, int32_t height, uint32_t px_len)
{
    if (!video_handle)  return -1;

    int32_t x, y;
    uint8_t r, g, b;
    uintptr_t off;

    for (y = y_orig; y < height + y_orig; y++)
        for (x = x_orig; x < width + x_orig; x++) {
            if (y >= current_mode.height || x >= current_mode.width || x < 0 || y < 0)
                continue;
            off = y * current_mode.width + x;
            if (off >= video_map.size)
                continue;
            r = px_len > 2 ? color >> 16 : (uint8_t)color;
            g = px_len > 2 ? (color >> 8) & 0xff : (uint8_t)color;
            b = px_len > 2 ? color & 0xff : (uint8_t)color;

            ((uint32_t*)video_map.base)[off] = (((((0xff << 8) | r) << 8) | g) << 8) | b;
        }

    return 0;
}
#pragma GCC pop_options
