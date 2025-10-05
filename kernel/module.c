#include <kernel/inari.h>
#include <kernel/errno.h>
#include <kernel/module.h>

#include <misc/string.h>

extern char __start_modules;
extern char __stop_modules;

int modules_init()
{
    int ret;

    /* Initialize all registered modules */
    for (module_t *dev = (module_t *)&__start_modules;
         dev < (module_t *)&__stop_modules;
         dev++)
    {
        if (!dev)
            continue;
        
        if (dev->probe && (ret = dev->probe()) != 0)
        {
#ifdef CONFIG_DEBUG
            printk("%s: %s", dev->name, ERRNO_STRING[-ret]);
#endif
        }
    }

    return 0;
}
