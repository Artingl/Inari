#include <kernel/inari.h>
#include <kernel/errno.h>
#include <kernel/module.h>
#include <kernel/event.h>

#include <misc/string.h>

extern char __start_modules;
extern char __stop_modules;

static int event_handler(event_t event)
{
    int res = EVENT_HANDLED;

    /* Broadcast the event to all loaded modules */
    for (module_metadata_t *dev = (module_metadata_t *)&__start_modules;
         dev < (module_metadata_t *)&__stop_modules;
         dev++)
    {
        if (dev->module->is_loaded && dev->module->event_bus)
        {
            res = dev->module->event_bus(event);
            if (res != EVENT_HANDLED)
                break;
        }
    }

    return res;
}

int modules_init()
{
    int ret;

    event_bus_subscribe(event_handler);

    /* Initialize all registered modules */
    for (module_metadata_t *dev = (module_metadata_t *)&__start_modules;
         dev < (module_metadata_t *)&__stop_modules;
         dev++)
    {
        if (dev->module->probe && (ret = dev->module->probe()) != 0)
        {
#ifdef CONFIG_DEBUG
            printk("%s: error %d", dev->name, ret);
#endif
        }

        event_bus_broadcast((event_t){
            .type = EVENT_MODULE_LOAD,
            .as = { .custom = dev->name }
        });
        dev->module->is_loaded = 1;
    }

    return 0;
}
