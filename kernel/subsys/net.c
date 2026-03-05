#ifdef CONFIG_SUBSYS_NET

#include <kernel/inari.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>
#include <kernel/subsys/net.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/errno.h>

#include <misc/string.h>
#include <misc/list.h>

static LIST_HEAD(net_devices);
static int is_initialized = 0;

static struct char_ops ops = {
    // .ioctl = &net_ioctl
};

int net_init(void) {
    register_chardev_group(NET_DRIVER, "net");
    is_initialized = 1;
    return 0;
}

int net_add_device(dev_t *dev, struct net_ops *net_ops, uint8_t *mac, uint16_t mtu, const char *name) {
    if (!net_ops || !name || !is_initialized || !mac)
        return -EINVAL;

    int res;
    struct net_device *device = (struct net_device *)kmalloc(sizeof(struct net_device));
    if (!device)
        return -ENOMEM;

    strcpy(device->name, name);
    memcpy(device->info.mac_addr, mac, sizeof(device->info.mac_addr));
    device->name[DEV_NAME_SIZE] = '\0';
    device->info.mtu = mtu;
    device->ops = net_ops;

    if ((res = register_chardev(NET_DRIVER, &ops, device, &device->dev)) != 0) {
        kfree(device);
        return res;
    }

    list_add(&device->list, &net_devices);
    if (dev)
        *dev = device->dev;

    kprintf("net: %s initialized; MAC %2x:%2x:%2x:%2x:%2x:%2x", name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return 0;
}

int net_remove_device(dev_t dev) {
    if (!is_initialized)
        return -EINVAL;
    struct device *chardev = char_get(dev);
    if (!chardev)
        return -ENODEV;
    if (chardev->driver_data) {
        list_del(&((struct net_device *)chardev->driver_data)->list);
        kfree(chardev->driver_data);
    }
    return unregister_chardev(dev);
}

int net_is_active(dev_t dev) {
    return 1;
}

int net_rx_packet(dev_t dev, void* data, uint32_t length) {
#ifdef CONFIG_DEBUG
    kprintf("net: rx data 0x%x, ln 0x%x", data, length);
#endif


    return 0;
}

#endif
