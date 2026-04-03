#ifndef _INARI_NET_IPV4
#define _INARI_NET_IPV4

#include <kernel/subsys/net.h>
#include <misc/types.h>

struct ipv4_packet {
    uint16_t version : 4;
    uint16_t ihl : 4;
    uint16_t tos : 8;
    uint16_t length;
    uint16_t id;
    uint16_t flgs : 3;
    uint16_t off_fragment : 13;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_address;
    uint32_t dest_address;
} __attribute__((packed));

int ipv4_rx_stack(struct net_link_layer_info *layer, struct ipv4_packet *packet, uint32_t ln);

int ipv4_tx_stack(struct net_socket *sock, struct net_link_layer_info *layer, struct net_ifaddr *ifaddr, uint8_t *dest_addr, size_t addr_sz,
                  uint8_t ipn, void *packet, uint32_t ln);

#endif
