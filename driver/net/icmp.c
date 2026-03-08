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
    uint32_t data;
} __attribute__((packed));

int icmp_rx_handler(struct net_network_layer_info *layer, struct icmp_header *packet, uint32_t ln) {
    struct icmp_header *icmp;

    // kprintf("icmp: packet %u %u %u %d %d", icmp->type, icmp->code, icmp->checksum, packet.ln,
    //         sizeof(struct icmp_header));

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

    default:
        return NET_DROPPED;
    }

    return NET_OK;
}

static int net_icmp_probe() {
    return net_define_protocol(NET_IPN_ICMP, (struct net_protocol){.rx = (net_protocol_rx)&icmp_rx_handler});
}

static void net_icmp_cleanup() { net_free_protocol(NET_IPN_ICMP); }

module_t net_icmp_module = {.probe = net_icmp_probe, .cleanup = net_icmp_cleanup};

module_register("net_icmp", net_icmp_module);

#endif
#endif
