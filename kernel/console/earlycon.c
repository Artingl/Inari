#include <kernel/console/earlycon.h>
#include <kernel/errno.h>
#include <kernel/inari.h>

#include <misc/string.h>

extern char __start_earlycon;
extern char __stop_earlycon;

static earlycon_device_t *device_in_use = NULL;

int earlycon_init() {
    earlycon_device_t *fallback = NULL;
    char device[ARG_MAX_LEN];
    parse_cmdline_argument("earlycon", &device[0]);

    /* Initialize any first available earlycon device specified in cmdline */
    for (earlycon_device_t *dev = (earlycon_device_t *)&__start_earlycon; dev < (earlycon_device_t *)&__stop_earlycon;
         dev++) {
        /* Save the first found dev as fallback if none was specified in earlycon cmdline */
        if (!fallback)
            fallback = dev;

        if (strcmp(dev->name, device))
            continue;

        if (dev->probe && dev->probe() == 0) {
            device_in_use = dev;

            kprintf("earlycon: using device %s as earlycon", dev->name);
            return 0;
        }
    }

    /* None earlycon device was probed, check that we have fallback */
    if (fallback && fallback->probe && fallback->probe() == 0) {
        device_in_use = fallback;
        kprintf("earlycon: using fallback device %s as earlycon", fallback->name);
        return 0;
    }

    return -EINVAL;
}
