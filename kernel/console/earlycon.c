#include <kernel/inari.h>
#include <kernel/console/earlycon.h>
#include <kernel/errno.h>

#include <misc/string.h>

extern char __start_earlycon;
extern char __stop_earlycon;

static earlycon_device_t *device_in_use = NULL;

int earlycon_init()
{
    char device[KERN_ARG_MAX_LEN];
    parse_cmdline_argument("earlycon", &device[0]);

    // Initialize any first available earlycon device specified in cmdline
    for (earlycon_device_t *dev = (earlycon_device_t *)&__start_earlycon;
         dev < (earlycon_device_t *)&__stop_earlycon;
         dev++)
    {
        if (!dev || strcmp(dev->name, device))
            continue;

        if (dev->probe && dev->probe() == 0)
        {
            device_in_use = dev;
            printk("earlycon: using %s as earlycon", dev->name);
            return 0;
        }
    }
    
    return -EINVAL;
}

