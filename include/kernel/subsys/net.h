#ifndef _INARI_NET_H
#define _INARI_NET_H

#include <misc/types.h>

#include <kernel/sync/spinlock.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>

struct net_ops {
    int (*tx)(struct net_device *bdev, void *packet, uint32_t length);
    int (*ioctl)(struct net_device *bdev, unsigned long req, void *arg);
} __attribute__((packed));

struct net_device_info {
    uint8_t mac_addr[6];
    uint16_t mtu;

    uint32_t flags;
    uint8_t link_state; // 1 = Cable plugged in, 0 = Cable unplugged
};

struct net_device {
    char name[DEV_NAME_SIZE + 1];
    struct net_device_info info;
    struct net_ops *ops;
    dev_t dev;

    struct list_head list;
} __attribute__((packed));

#define NET_ETHTYPE_IPV4 0x0800
#define NET_ETHTYPE_ARP  0x0806

#define NET_HANDLED 0
#define NET_DROPPED -1

#define NET_IPN_ICMP 0x01
#define NET_IPN_TCP  0x07
#define NET_IPN_UDP  0x11

__attribute__((unused)) static const char *ipnstr[] = {
    [NET_IPN_ICMP] = "ICMP",
    [NET_IPN_TCP] = "TCP",
    [NET_IPN_UDP] = "UDP",
};

struct net_frame {
    uint8_t set;
    void *data;
    uint32_t length;
    spinlock_t frame_lock;
};

struct ethernet_frame {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ether_type;
    uint8_t data[];
} __attribute__((packed));

int net_init(void);
int net_add_device(dev_t *dev, struct net_ops *ops, uint8_t *mac, uint16_t mtu, const char *name);
int net_remove_device(dev_t dev);
int net_is_active(dev_t dev);
int net_rx_packet(dev_t dev, void *data, uint32_t length);

#endif
