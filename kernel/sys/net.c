#ifdef CONFIG_SUBSYS_NET

#include <kernel/inari.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>
#include <kernel/sys/net.h>

int net_init(void) {
    register_chardev_group(NET_DRIVER, "net");

    return 0;
}

// int register_net_device()
#endif