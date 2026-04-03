#include <kernel/inari.h>
#include <kernel/net/arp.h>

#include <misc/list.h>
#include <misc/string.h>
#include <misc/types.h>

int arp_tx_stack(struct net_link_layer_info *layer, struct net_ifaddr *ifaddr, uint8_t *dest_addr, size_t addr_sz,
                 uint8_t ipn, void *packet, uint32_t ln) {
    return NET_DROPPED;
}

int arp_rx_stack(struct net_link_layer_info *layer, void *packet, uint32_t ln) {
    struct arp_resolv_ipv4_packet *resolv = packet;
    struct arp_resolv_ipv4_packet *response;
    uint16_t protocol_type = resolv->protocol_type;

    protocol_type = bigend16(protocol_type);

    if (protocol_type != NET_ETHTYPE_IPV4)
        return NET_DROPPED;

    /* Allocate memory for response */
    if (layer->ops->req_buf(layer, (void **)&response, ln) != 0)
        /* Unable to allocate buffer for answer */
        return NET_DROPPED;

    struct list_head *pos;
    struct net_ifaddr *entry;
    list_for_each(pos, &layer->dev->ifaddrs) {
        entry = list_entry(pos, struct net_ifaddr, list);
        if (memcmp(entry->address.ipv4, (uint8_t *)&resolv->destination_ipv4, 4) == 0)
            goto match;
    }

    /* Didn't match the IPv4 address */
    return NET_DROPPED;
match:

    response->hardware_info = resolv->hardware_info;
    response->protocol_type = resolv->protocol_type;
    response->hardware_ln = resolv->hardware_ln;
    response->protocol_ln = resolv->protocol_ln;
    response->operation = 0x0002; // ARP reply

    /* Set the destination to the requester */
    memcpy(response->destination_hardware_addr, resolv->source_hardware_addr, 6);
    response->destination_ipv4 = resolv->source_ipv4;

    /* Now set the source hardware/protocol addresses */
    memcpy(response->source_hardware_addr, layer->device_mac, 6);
    memcpy((uint8_t *)&response->source_ipv4, entry->address.ipv4, 4);

#ifdef CONFIG_LITTLE_ENDIAN
    response->operation = swap_endian16(response->operation);
#endif

    return layer->ops->tx(layer, response, ln);
}
