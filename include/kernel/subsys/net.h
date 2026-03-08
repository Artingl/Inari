#ifndef _INARI_NET_H
#define _INARI_NET_H

#include <misc/types.h>

#include <kernel/sync/spinlock.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>

#define NET_IOCTL_INFO          0
#define NET_IOCTL_IFADDR_NEXT   1
#define NET_IOCTL_ATTACH_IFADDR 2
#define NET_IOCTL_DETACH_IFADDR 3

struct net_ops {
    int (*tx)(struct net_device *bdev, void *packet, uint32_t length);
    int (*ioctl)(struct net_device *bdev, unsigned long req, void *arg);
} __attribute__((packed));

struct net_device_info {
    char name[DEV_NAME_SIZE + 1];
    uint8_t mac_addr[6];
    uint16_t mtu;

    uint32_t flags;
    uint8_t link_state; // 1 = Cable plugged in, 0 = Cable unplugged
};

struct net_ifaddr {
#define NET_IF_INET  0 // IPv4
#define NET_IF_INET6 1 // IPv6

    uint16_t family;

    union {
        uint8_t ipv4[4];
        uint8_t ipv6[16];
    } address;

    union {
        uint8_t ipv4[4];
        uint8_t ipv6[16];
    } netmask;

    union {
        uint8_t ipv4[4];
        uint8_t ipv6[16];
    } gateway;

    struct net_device *device;
    struct list_head list;
};

struct net_device {
    struct net_device_info info;
    struct net_ops *ops;
    dev_t dev;

    struct list_head ifaddrs;
    struct list_head list;
} __attribute__((packed));

#define NET_ETHTYPE_IPV4 0x0800
#define NET_ETHTYPE_ARP  0x0806

#define NET_OK      0
#define NET_DROPPED -1

#define NET_IPN_ICMP 0x01
#define NET_IPN_TCP  0x06
#define NET_IPN_UDP  0x11

__attribute__((unused)) static const char *ipnstr[] = {
    [NET_IPN_ICMP] = "ICMP",
    [NET_IPN_TCP] = "TCP",
    [NET_IPN_UDP] = "UDP",
};

struct net_frame {
    uint8_t set;
    dev_t dev;
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

struct net_layer_ops {
    /* Use this layer to transmit data.
     * Note: data passed in `packet` variable MUST be allocated via req_buf. After packet transmission the buffer will
     * be deallocated automatically.
     */
    int (*tx)(void *layer_info, void *packet, uint32_t ln);

    /* Used to request buffer of asked size. This function will account for all additional required space in buffer
     * (ethernet frame + ipv4/ipv6 packet, etc.) and will put it right before the provided pointer
     */
    int (*req_buf)(void *layer_info, void **result, uint32_t ln);
};

struct net_link_layer_info {
    struct net_device *dev; // source device
    struct ethernet_frame *layer;

    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint8_t device_mac[6]; // receiver NIC MAC
    uint16_t ether_type;

    /* Layer specific ops */
    struct net_layer_ops *ops;
};

struct net_network_layer_info {
    void *layer;                            // ipv4/ipv6 packet
    struct net_link_layer_info *link_layer; // underlying link layer
    struct net_ifaddr *ifaddr;

    /* Layer specific ops */
    struct net_layer_ops *ops;
};

typedef int (*net_protocol_rx)(struct net_network_layer_info *layer, void *packet, uint32_t ln);

struct net_protocol {
    net_protocol_rx rx;
};

struct net_protocol *net_invoke_protocol(uint8_t ipn);
int net_init(void);
int net_attach_ifaddr(dev_t dev, struct net_ifaddr ifaddr);
int net_detach_ifaddr(dev_t dev, struct net_ifaddr ifaddr);
int net_define_protocol(uint8_t ipn, struct net_protocol protocol);
void net_free_protocol(uint8_t ipn);
int net_add_device(dev_t *dev, struct net_ops *ops, uint8_t *mac, uint16_t mtu, const char *name);
int net_remove_device(dev_t dev);
int net_is_active(dev_t dev);
int net_tx_packet(dev_t dev, void *packet, uint32_t length);
int net_rx_packet(dev_t dev, void *data, uint32_t length);

__attribute__((unused)) static inline uint16_t net_checksum(void *data, uint32_t ln) {
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < ln >> 1; i++) {
#ifdef CONFIG_LITTLE_ENDIAN
        checksum += swap_endian16(((uint16_t *)data)[i]);
#else
        checksum += ((uint16_t *)data)[i];
#endif
    }
    return (~((checksum & 0xffff) + ((checksum >> 16) & 0xf))) & 0xffff;
}

#endif
