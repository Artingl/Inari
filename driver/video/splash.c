#ifdef CONFIG_DRV_SPLASH
#ifdef CONFIG_SUBSYS_VIDEO

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/module.h>
#include <kernel/subsys/video.h>

#include "splash.h"

static void draw(dev_t dev) {
    int res;
    /* Ignore if not video group */
    if (!ISGROUP(DRVID(dev), DRIVER_VIDEO_GROUP))
        return;

    struct video_device *video_dev;
    /* Try to get the very first video device */
    video_dev = video_get(dev);
    if (!video_dev) {
        kprintf("splash: No video devices available.");
        return;
    }

    if (!video_dev->ops->blit || !video_dev->ops->mode_info) {
        kprintf("splash: Function not implemented.");
        return;
    }

    /* Get current mode */
    struct video_mode_info info;
    if ((res = video_dev->ops->mode_info(video_dev, &info)) != 0) {
        kprintf("splash: %s.", errno(res));
        return;
    }

    /* Render on screen */
    struct video_blit blit_info = {.x = (info.width >> 1) - (LOGO_WIDTH >> 1),
                                   .y = (info.height >> 1) - (LOGO_HEIGHT >> 1),
                                   .width = LOGO_WIDTH,
                                   .height = LOGO_HEIGHT,
                                   .format = VIDEO_R8G8B8_FORMAT,
                                   .buffer = (uint8_t *)&splash_logo};
    if ((res = video_dev->ops->blit(video_dev, &blit_info)) != 0) {
        kprintf("splash: %s.", errno(res));
        return;
    }

    kprintf("splash: drawn on device %s", video_dev->name);
}

static int splash_probe() { return 0; }

static void splash_cleanup() {}

static int splash_event_handler(event_t event) {
    switch (event.type) {
    case EVENT_LOAD_CHARDEV:
        draw(event.as.dev);
    }

    return EVENT_HANDLED;
}

module_t splash_module = {.probe = splash_probe, .cleanup = splash_cleanup, .event_bus = splash_event_handler};

module_register("splash", splash_module);

#endif
#endif
