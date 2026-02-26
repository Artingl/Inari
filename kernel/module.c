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
        if ((dev->module->flags & MODULE_FLAG_IS_LOADED) && dev->module->event_bus)
        {
            res = dev->module->event_bus(event);
            if (res != EVENT_HANDLED)
                break;
        }
    }

    return res;
}

void modules_cleanup()
{
    for (module_metadata_t *dev = (module_metadata_t *)&__start_modules;
         dev < (module_metadata_t *)&__stop_modules;
         dev++)
    {
        if (dev->module->flags & MODULE_FLAG_IS_LOADED)
        {
            event_bus_broadcast((event_t){
                .type = EVENT_MODULE_UNLOAD,
                .as = { .custom = dev->name }
            });
            if (dev->module->cleanup)
                dev->module->cleanup();
            dev->module->flags &= ~MODULE_FLAG_IS_LOADED;
        }
    }
}

int modules_insmod(const char *name)
{
    int ret = -1;
    for (module_metadata_t *dev = (module_metadata_t *)&__start_modules;
         dev < (module_metadata_t *)&__stop_modules;
         dev++)
    {
        if (!(dev->module->flags & MODULE_FLAG_IS_LOADED) && strcmp(dev->name, name) == 0)
        {
            if (dev->module->probe && (ret = dev->module->probe()) != 0)
            {
                kprintf("%s: Module load error: %s.", dev->name, errstr[-ret] ? errstr[-ret] : "Invalid error");
                return -1;
            }

            event_bus_broadcast((event_t){
                .type = EVENT_MODULE_LOAD,
                .as = { .custom = dev->name }
            });
            dev->module->flags |= MODULE_FLAG_IS_LOADED;
            return ret;
        }
    }

    return -1;
}

int modules_ls(int idx, char *name, uintptr_t *ptr, uint32_t *flags)
{
    size_t i = 0;
    for (module_metadata_t *dev = (module_metadata_t *)&__start_modules;
         dev < (module_metadata_t *)&__stop_modules;
         dev++)
    {
        if (i++ >= idx)
        {
            if (ptr)    *ptr = (uintptr_t)dev->module->probe;
            if (name)    memcpy((void*)name, (void*)dev->name, strlen(dev->name) + 1);
            if (flags)  *flags = dev->module->flags;
            return 1;
        }
    }

    return 0;
}

int modules_rmmod(const char *name)
{
    for (module_metadata_t *dev = (module_metadata_t *)&__start_modules;
         dev < (module_metadata_t *)&__stop_modules;
         dev++)
    {
        if ((dev->module->flags & MODULE_FLAG_IS_LOADED) && strcmp(dev->name, name) == 0)
        {
            event_bus_broadcast((event_t){
                .type = EVENT_MODULE_UNLOAD,
                .as = { .custom = dev->name }
            });
            if (dev->module->cleanup)
                dev->module->cleanup();
            dev->module->flags &= ~MODULE_FLAG_IS_LOADED;
            return 0;
        }
    }

    return -1;
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
        dev->module->flags |= MODULE_FLAG_BUILTIN;
        if (dev->module->flags & MODULE_FLAG_LAZY_LOAD)
            continue;

        if (dev->module->probe && (ret = dev->module->probe()) != 0)
        {
            kprintf("%s: Module load error: %s.", dev->name, errstr[-ret] ? errstr[-ret] : "Invalid error");
            continue;
        }

        event_bus_broadcast((event_t){
            .type = EVENT_MODULE_LOAD,
            .as = { .custom = dev->name }
        });
        dev->module->flags |= MODULE_FLAG_IS_LOADED;
    }

    return 0;
}
