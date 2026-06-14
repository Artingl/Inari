#ifndef _INARI_NET_ARP
#define _INARI_NET_ARP

#include <kernel/subsys/net.h>
#include <misc/types.h>

#define ARP_REPLY 0x0002
#define ARP_REQ   0x0001

struct arp_resolv_ipv4_packet {
    uint16_t hardware_type, protocol_type;
    uint8_t hardware_ln, protocol_ln;
    uint16_t operation;
    uint8_t source_hardware_addr[6]; // typically 6 for ethernet
    uint32_t source_ipv4;
    uint8_t destination_hardware_addr[6]; // typically 6 for ethernet
    uint32_t destination_ipv4;
} __attribute__((packed));

/* Asks on the network what MAC address does a specific IPv4 address have */
int arp_resolve_ipv4(struct net_link_layer_info *layer, struct net_ifaddr *ifaddr, uint8_t *ipv4, uint8_t *result_mac);

int arp_rx_stack(struct net_link_layer_info *layer, void *packet, uint32_t ln);

int arp_tx_stack(struct net_socket *sock, struct net_link_layer_info *layer, uint8_t *origin_addr,
                 struct net_sock_addr dest_addr, uint8_t ipn, void *packet, uint32_t ln);

#endif
