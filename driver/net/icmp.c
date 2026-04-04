#ifdef CONFIG_SUBSYS_NET
#ifdef CONFIG_NET_ICMP

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/module.h>
#include <kernel/net/ipv4.h>
#include <kernel/subsys/net.h>

#include <misc/string.h>
#include <misc/types.h>

struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;

    union {
        struct {
            uint16_t identifier;
            uint16_t seq;
        } ping_pong;
    } as;
} __attribute__((packed));

int icmp_rx_handler(struct net_network_layer_info *layer, struct icmp_header *packet, uint32_t ln) {
    struct icmp_header *icmp;
    struct net_sock_addr origin;

    switch ((packet->type << 8 | packet->code)) {
    /* ICMP echo request (type 8, code 0) */
    case 0x0800:

        /* Allocate buffer for answer back */
        if (layer->ops->req_buf(layer, (void **)&icmp, ln) != 0)
            /* Unable to allocate buffer for answer */
            return NET_DROPPED;

        /* Copy the packet and respond with echo reply (type 0, code 0) */
        memcpy(icmp, packet, ln);
        icmp->checksum = 0;
        icmp->type = 0;
        icmp->checksum = bigend16(net_checksum(icmp, ln));
        return layer->ops->tx(layer, icmp, ln);

    /* Any other send to the socket if any */
    default:
        origin.addr_ln = layer->origin_addr_ln;
        origin.identifier = bigend16(packet->as.ping_pong.identifier);
        memcpy(origin.address, layer->origin, origin.addr_ln);
        return net_sock_fill_datagram(origin, origin.identifier, packet, ln);
    }

    return NET_OK;
}

static int icmp_tx_handler(struct net_socket *sock, struct net_sock_addr addr, struct net_network_layer_info *layer,
                           void *packet, uint32_t ln) {
    void *tx_data;

    /* Allocate buffer for data layering */
    if (layer->ops->req_buf(layer, (void **)&tx_data, ln) != 0)
        /* Unable to allocate buffer for answer */
        return NET_DROPPED;

    /* Set the identifier so we later dont lose the packet */
    struct icmp_header *header = packet;
    sock->identifier = sock->owner;
    header->as.ping_pong.identifier = bigend16((uint16_t)sock->owner);
    header->checksum = 0;
    header->checksum = bigend16(net_checksum(packet, ln));

    /* Transmit data */
    memcpy(tx_data, packet, ln);
    return layer->ops->tx(layer, tx_data, ln);
}

static int net_icmp_probe() {
    return net_define_protocol(NET_IPN_ICMP, (struct net_protocol){.is_privileged = 0,
                                                                   .rx = (net_protocol_rx_t)&icmp_rx_handler,
                                                                   .tx = (net_protocol_tx_t)&icmp_tx_handler});
}

static void net_icmp_cleanup() { net_free_protocol(NET_IPN_ICMP); }

module_t net_icmp_module = {.probe = net_icmp_probe, .cleanup = net_icmp_cleanup};

module_register("net_icmp", net_icmp_module);

#endif
#endif
