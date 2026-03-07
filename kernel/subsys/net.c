#ifdef CONFIG_SUBSYS_NET

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/net/arp.h>
#include <kernel/net/ipv4.h>
#include <kernel/proc/sched.h>
#include <kernel/subsys/net.h>
#include <kernel/sync/spinlock.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>

#include <misc/list.h>
#include <misc/ring.h>
#include <misc/string.h>
#include <misc/types.h>

#define RING_LN 10

static struct net_frame net_ring_buffer[RING_LN] = {0};

static LIST_HEAD(net_devices);
static int is_initialized = 0;
static tid_t queue_thread;

static void net_queue_thread(void *_) {
    while (!is_initialized)
        ;

    struct net_frame *net_frame = NULL;
    struct ethernet_frame *frame;
    size_t i = 0;
    int res;
    do {
        /* Loop to find waiting frames in queue */
        while (!(net_frame = &net_ring_buffer[i++ % RING_LN])->set)
            sched_yield();

        /* Minimum size of ethernet frame is 64 bytes */
        if (net_frame->length < 64) {
            res = NET_DROPPED;
            goto end;
        }

        /* Now handle the actual packet! */
        frame = net_frame->data;
        #ifdef CONFIG_LITTLE_ENDIAN
        switch (swap_endian16(frame->ether_type)) {
#else
        switch (frame->ether_type) {
#endif
        case NET_ETHTYPE_IPV4:
            res = ipv4_rx_stack(frame, net_frame->length);
            break;
        case NET_ETHTYPE_ARP:
            res = arp_rx_stack(frame, net_frame->length);
            break;
        default:
            res = NET_DROPPED;
        }

end:
#ifdef CONFIG_DEBUG
        if (res == NET_DROPPED)
            kprintf("net: dropped rx 0x%x, ln 0x%x; ether type: 0x%x", net_frame->data, net_frame->length,
                    swap_endian16(frame->ether_type));
#endif

        net_frame->set = 0;

        /* Don't forget to unlock, so net_rx can use this frame spot */
        spin_unlock(&net_frame->frame_lock);
    } while (1);
}

static struct char_ops ops = {
    // .ioctl = &net_ioctl
};

int net_init(void) {
    register_chardev_group(NET_DRIVER, "net");
    is_initialized = 1;
    return sched_create_thread(&queue_thread, &net_queue_thread, NULL, NULL, NULL);
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

int net_is_active(dev_t dev) { return 1; }

int net_rx_packet(dev_t dev, void *data, uint32_t length) {
    static size_t ring_off = 0;
    struct net_frame *frame = NULL;

    /* Find free frame to save the packet into */
    do {
        if (spin_try_lock(&net_ring_buffer[ring_off].frame_lock)) {
            frame = &net_ring_buffer[ring_off];
            ring_off = (ring_off + 1) % RING_LN;

            /* Ignore unlocked frame spot if already in use */
            if (frame->set) {
                spin_unlock(&frame->frame_lock);
                frame = NULL;
                continue;
            }

            /* Use this frame in buffer */
            break;
        }

        /* Try next spot */
        ring_off = (ring_off + 1) % RING_LN;
    } while (1);

    if (!frame) {
#ifdef CONFIG_DEBUG
        kprintf("net: drop packet 0x%x, ln 0x%x", data, length);
#endif
        /* Drop frame */
        return -1;
    }

    frame->set = 1;
    frame->data = data;
    frame->length = length;

    /* Keep locked, the queue thread will unlock when frame is free */
    // spin_unlock(&frame->frame_lock);
    return 0;
}

#endif
