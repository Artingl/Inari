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
    uint16_t flgs_off_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_address;
    uint32_t dest_address;
    uint8_t data[];
} __attribute__((packed));

typedef int (*ipv4_handler)(struct ipv4_packet *packet, size_t ln);

int ipv4_subscribe(uint8_t ipn, ipv4_handler handler);
void ipv4_unsubscribe(uint8_t ipn);
int ipv4_rx_stack(struct ethernet_frame *frame, size_t ln);

#endif
