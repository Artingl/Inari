#ifndef _INARI_NET_H
#define _INARI_NET_H

#include <misc/types.h>

#include <kernel/proc/proc.h>
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

struct net_route_entry {
    /* TODO: only ipv4 for now. Look at ifaddr for reference for future */
    uint8_t dest_network[4];
    uint8_t netmask[4];
    uint8_t gateway[4];
    uint8_t is_default;
    struct net_ifaddr *ifaddr;
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
#define NET_ERR     -2

#define NET_IPN_ICMP 0x01
#define NET_IPN_TCP  0x06
#define NET_IPN_UDP  0x11

/* IPN = IP number */
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

struct net_sock_addr {
    size_t addr_ln; // always 4

    uint8_t address[4]; // ipv4
    uint16_t identifier;
} __attribute__((packed));

struct net_network_layer_info {
    void *layer;                            // ipv4/ipv6 packet
    struct net_link_layer_info *link_layer; // underlying link layer
    struct net_ifaddr *ifaddr;

    uint8_t origin_addr_ln;
    uint8_t origin[16];

    /* Layer specific ops */
    struct net_layer_ops *ops;
};

/* TX/RX flow */
typedef int (*net_protocol_rx_t)(struct net_network_layer_info *layer, void *packet, uint32_t ln);

/* Identifier is the unique ID to know where the answer from this TX (if any) should go (e.g. specific socket) */
struct net_socket;
struct net_sock_addr;
typedef int (*net_protocol_tx_t)(struct net_socket *sock, struct net_sock_addr addr,
                                 struct net_network_layer_info *layer, void *packet, uint32_t ln);

/* Identifier could be a port, or similar in different protocols */
typedef int (*net_protocol_transport_t)(struct net_socket *sock, struct net_sock_addr addr);
typedef int (*net_protocol_close_t)(struct net_socket *sock);

struct net_protocol {
    uint8_t is_privileged; /* Does it require to be privileged to use this protocol (e.g. in future something like root)
                            */

    net_protocol_rx_t rx;
    net_protocol_tx_t tx;
    net_protocol_transport_t bind;
    net_protocol_transport_t connect;
    net_protocol_close_t close;
};

typedef int64_t net_handle_t;

/* Used in syscalls */
#define NET_SYS_CREATE   0x0
#define NET_SYS_FREE     0x1
#define NET_SYS_SENDTO   0x2
#define NET_SYS_RECVFROM 0x3
#define NET_SYS_BIND     0x4
#define NET_SYS_CONNECT  0x5
struct net_sys_command {
    uint8_t id;
    union {
        struct {
            uint8_t ipn;
            uint16_t ethtype;
        } create;

        struct {
            struct net_sock_addr addr;
        } transport;

        /* Data flow (recv/send) */
        struct {
            void *buffer;
            size_t buffer_sz;
            struct net_sock_addr addr;
            struct net_sock_addr *from;

            /* For recv */
            uint32_t timeout_us;
            uint16_t flags;
        } flow;
    } as;
} __attribute__((packed));

struct net_buf {
    struct net_sock_addr origin;
    uint16_t ln;
    uint8_t payload[1500];
    struct list_head list;
};

int net_buf_free(struct net_buf *buf);
int net_buf_alloc(struct net_buf **result);

struct net_socket {
    net_handle_t handle;
    uint8_t protocol_data[32];
    uint8_t ipn; // ICMP, UDP, TCP, ...
    uint8_t is_stream; // stream/datagram
    uint16_t ethtype;
    uint16_t identifier; // Could be a port, ICMP identifier, anything
    pid_t owner;
    union {
        struct {
            struct net_buf *buf;
        } datagram;

        struct {
            struct net_sock_addr origin;
            uint16_t head;
            uint16_t tail;
            uint8_t ring_buffer[65535];
        } stream;
    } rx;
    struct list_head list;
};

int net_syscall(pid_t caller, struct net_sys_command *command, net_handle_t *sock_handle);

/* Used by proc.c to cleanup net handles for a given process */
void net_proc_cleanup(struct process *proc);

/* Allocates new net_buf in socket and either fills the stream of datagram part of it */
int net_sock_fill_stream(struct net_sock_addr origin, uint16_t dest_identifier, void *packet, uint32_t ln);
int net_sock_fill_datagram(struct net_sock_addr origin, uint16_t dest_identifier, void *packet, uint32_t ln);

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
