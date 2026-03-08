#ifdef CONFIG_SUBSYS_NET

#include <kernel/inari.h>
#include <kernel/net/ipv4.h>
#include <kernel/subsys/net.h>

#include <misc/string.h>
#include <misc/types.h>

static int ipv4_tx_layer(void *layer_info, void *packet, uint32_t ln) {
    if (!packet)
        return -1;

    /* Construct IPv4 packet and transmit further */
    struct net_network_layer_info *info = layer_info;
    struct ipv4_packet *ipv4_layer_packet = info->layer;

    /* Step back in packet buffer to access memory allocated for IPv4 packet by `req_buf` */
    struct ipv4_packet *ipv4_packet = (struct ipv4_packet *)(packet - sizeof(struct ipv4_packet));
    memcpy(ipv4_packet, ipv4_layer_packet, 20);
    ipv4_packet->length = bigend16(20 + ln);
    ipv4_packet->off_fragment = 0;
    ipv4_packet->flgs = 0;
    ipv4_packet->id = 0;
    ipv4_packet->ttl = 64;

    ipv4_packet->src_address = 0x8501a8c0;
    ipv4_packet->dest_address = ipv4_layer_packet->src_address;

    /* Finally, calculate checksum */
    ipv4_packet->checksum = 0;
    ipv4_packet->checksum = bigend16(net_checksum(ipv4_packet, ln + sizeof(struct ipv4_packet)));

    return info->link_layer->ops->tx(info->link_layer, ipv4_packet, ln + sizeof(struct ipv4_packet));
}

static int ipv4_req_buf(void *layer_info, void **result, uint32_t ln) {
    struct net_network_layer_info *info = layer_info;
    int res = info->link_layer->ops->req_buf(info->link_layer, result, ln + sizeof(struct ipv4_packet));
    if (res == 0)
        *result += sizeof(struct ipv4_packet);
    return res;
}

static struct net_layer_ops ipv4_network_layer_ops = {.tx = &ipv4_tx_layer, .req_buf = &ipv4_req_buf};

int ipv4_rx_stack(struct net_link_layer_info *layer, struct ipv4_packet *packet, uint32_t ln) {
    /* Minimum size is 20 bytes for IPv4 packet */
    if (ln < 20) {
        return NET_DROPPED;
    }

#ifdef CONFIG_LITTLE_ENDIAN
    /* Dirty work here to get ihl and version on little endian */
    uint16_t header_size = ((swap_endian16(*(uint16_t *)packet) >> 8) & 0xf) * 4;
    if ((swap_endian16(*(uint16_t *)packet) >> 12) != 4)
#else
    uint16_t header_size = packet->ihl * 4;
    if (packet->version != 4)
#endif
        /* Different packet version */
        return NET_DROPPED;

        /* TODO: checksum validation */

#ifdef CONFIG_LITTLE_ENDIAN
    /* Some more dirty work here to get flags on little endian */
    if ((swap_endian16(*(((uint16_t *)packet) + 3)) >> 13) & (1 << 2))
#else
    if (packet->flgs & (1 << 2))
#endif
        /* TODO: Packet is fragmented */
        return NET_DROPPED;

    struct net_network_layer_info network_layer = {
        .link_layer = layer, .layer = packet, .ops = &ipv4_network_layer_ops};

    uint32_t protocol_ln = bigend16(packet->length) - header_size;

    struct net_protocol *protocol = net_invoke_protocol(packet->protocol);

    /* Send the packet to correct protocol handler */
    if (protocol)
        return protocol->rx(&network_layer, ((uint8_t *)packet) + header_size, protocol_ln);
    return NET_DROPPED;
}

#endif
