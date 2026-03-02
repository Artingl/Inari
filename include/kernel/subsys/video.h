#ifndef _INARI_VIDEO_H
#define _INARI_VIDEO_H

#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>

#include <misc/list.h>
#include <misc/types.h>

#define VIDEO_IOCTL_INFO           0 // Current mode info
#define VIDEO_IOCTL_MODE_SWITCH    1 // Switch to different mode
#define VIDEO_IOCTL_MODE_FIND_NEXT 2 // Iterate through all modes
#define VIDEO_IOCTL_BLIT           4 // Blit using video device
#define VIDEO_IOCTL_DISABLE        5 // Disable the video device (e.g. switch to text mode in VESA)
#define VIDEO_IOCTL_FILL_RECT      6 // Fill a rect with dimensions and static color
#define VIDEO_IOCTL_MAP            7 // Map video memory to process's virtual memory and provide info

struct video_device;
struct video_mode_info;
struct video_blit;
struct video_fill_rect;
struct video_map_info;

struct video_ops {
    int (*mode_info)(struct video_device *device, struct video_mode_info *result);     // Get info about current mode
    int (*mode_switch)(struct video_device *device, struct video_mode_info *new_mode); // Switch to a new mode
    int (*mode_find_next)(struct video_device *device, struct video_mode_info *mode);  // Iterate through all modes
    int (*blit)(struct video_device *device, struct video_blit *blit_info);
    int (*fill_rect)(struct video_device *device, struct video_fill_rect *rect);
    void (*disable)(struct video_device *device); // Disable device (e.g. switch to text mode in VESA)
    int (*map_video)(struct video_device *device, struct video_map_info *mmap); // Map video memory to process's memory

} __attribute__((packed));

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
__attribute__((unused)) static uint8_t video_format_bpp[] = {
    [VIDEO_R8G8B8_FORMAT] = 24,
    [VIDEO_GRAY8_FORMAT] = 8,
};

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

struct video_device {
    char name[DEV_NAME_SIZE + 1];
    struct video_mode_info info;
    uintptr_t base;
    struct video_ops *ops;
    dev_t dev;

    struct list_head list;
} __attribute__((packed));

int video_init(void);
int video_add_device(dev_t *dev, const char *name, uintptr_t base, struct video_ops *ops);
int video_remove_device(dev_t dev);
int video_disable(void);
struct device *video_get(dev_t dev);

#endif