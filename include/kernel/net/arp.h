#ifndef _INARI_NET_ARP
#define _INARI_NET_ARP

#include <kernel/subsys/net.h>
#include <misc/types.h>

struct arp_resolv_ipv4_packet {
    uint16_t hardware_info, protocol_type;
    uint8_t hardware_ln, protocol_ln;
    uint16_t operation;
    uint8_t source_hardware_addr[6]; // typically 6 for ethernet
    uint32_t source_ipv4;
    uint8_t destination_hardware_addr[6]; // typically 6 for ethernet
    uint32_t destination_ipv4;
} __attribute__((packed));

int arp_rx_stack(struct net_link_layer_info *layer, void *packet, uint32_t ln);
int arp_tx_stack(struct net_link_layer_info *layer, struct net_ifaddr *ifaddr, uint8_t *dest_addr, size_t addr_sz,
                 uint8_t ipn, void *packet, uint32_t ln);

#endif
