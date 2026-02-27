#ifdef CONFIG_DRV_SPLASH

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/module.h>

static int video_subsystem_ready = 0;

static int splash_probe() { return 0; }

static void splash_cleanup() {}

static int splash_event_handler(event_t event) {
    if (video_subsystem_ready)
        return EVENT_HANDLED;

    /* Wait for the video subsystem to init */
    // switch (event.type)
    // {
    //     case EVENT_UNLOAD_BLKDEV:
    //         handle_dev_unload(event.as.dev);
    //         break;
    // }

    return EVENT_HANDLED;
}

module_t splash_module = {.probe = splash_probe, .cleanup = splash_cleanup, .event_bus = splash_event_handler};

module_register("splash", splash_module);

#endif