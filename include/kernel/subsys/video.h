#ifndef _INARI_VIDEO_H
#define _INARI_VIDEO_H

#include <kernel/sys/driver.h>
#include <kernel/sys/device.h>

#include <misc/types.h>

#define VIDEO_IOCTL_INFO                0   // Current mode info
#define VIDEO_IOCTL_MODE_SWITCH         1   // Switch to different mode
#define VIDEO_IOCTL_MODE_FIND_NEXT      2   // Iterate through all modes
#define VIDEO_IOCTL_BLIT                4   // Blit using video device

struct video_device;
struct video_mode_info;
struct video_blit;

struct video_ops
{
    int (*mode_info)(struct video_device *device, struct video_mode_info *result);               // Get info about current mode
    int (*mode_switch)(struct video_device *device, struct video_mode_info *new_mode);           // Switch to a new mode
    int (*mode_find_next)(struct video_device *device, struct video_mode_info *mode);           // Iterate through all modes
    int (*blit)(struct video_device *device, struct video_blit *blit_info);

} __attribute__((packed));

struct video_blit
{
#define VIDEO_R8G8B8_FORMAT   0
    uint8_t format;
    uint8_t *buffer;
    uint32_t x, y;
    uint32_t width, height;
} __attribute__((packed));

struct video_mode_info
{
    /* Mode info */
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    
    /* Video driver can set this value if this is the default mode for the display */
    uint8_t is_default; 

    /* Driver specific mode id */
    uint32_t mode_id;
} __attribute__((packed));

struct video_device
{
    char name[DEV_NAME_SIZE + 1];
    struct video_mode_info info;
    uintptr_t base;
    struct video_ops *ops;
    dev_t dev;
} __attribute__((packed));

int video_init(void);
int video_add_device(dev_t *dev, const char *name, uintptr_t base, struct video_ops *ops);
int video_remove_device(dev_t dev);
struct device *video_get(dev_t dev);

#endif