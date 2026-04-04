#ifndef _LIBC_NET_H
#define _LIBC_NET_H

#include <list.h>
#include <sys.h>
#include <types.h>

#define NET_IOCTL_INFO          0
#define NET_IOCTL_IFADDR_NEXT   1
#define NET_IOCTL_ATTACH_IFADDR 2
#define NET_IOCTL_DETACH_IFADDR 3

struct net_device_info {
    char name[DEV_NAME_SIZE + 1];
    uint8_t mac_addr[6];
    uint16_t mtu;

    uint32_t flags;
    uint8_t link_state; // 1 = Cable plugged in, 0 = Cable unplugged
};

struct net_sock_addr {
    size_t addr_ln; // always 4

    uint8_t address[4]; // ipv4
    uint16_t identifier;
} __attribute__((packed));

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

#define NET_ETHTYPE_IPV4 0x0800
#define NET_ETHTYPE_ARP  0x0806

#define NET_IPN_ICMP 0x01
#define NET_IPN_TCP  0x06
#define NET_IPN_UDP  0x11

int sockconnect(handle_t sock_handle, struct net_sock_addr addr);
int sockbind(handle_t sock_handle, struct net_sock_addr addr);
int sockcreate(handle_t *sock_handle, uint8_t ip_number, uint16_t ethtype);
int socksendto(handle_t sock_handle, void *data, size_t data_size, struct net_sock_addr addr);
/* Timeout of 0 will mean it is blocking until any response */
int sockrecvfrom(handle_t sock_handle, void *result_buffer, size_t result_size, struct net_sock_addr *from,
                 uint32_t timeout_us, uint16_t flags);
int sockclose(handle_t sock_handle);

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
