#ifdef CONFIG_SUBSYS_NET

#include <kernel/inari.h>
#include <kernel/net/arp.h>
#include <kernel/proc/sched.h>
#include <kernel/sync/spinlock.h>

#include <misc/list.h>
#include <misc/string.h>
#include <misc/types.h>

static spinlock_t arp_req_lock = {0};
static struct {
    uint8_t acquired;
    uint8_t delivered;
    uint8_t ipv4_source[4]; // The source IPv4 this req is waiting for
    struct arp_resolv_ipv4_packet result;
} arp_req_list[16] = {0};

static struct {
    uint8_t available;
    uint64_t timeout;

    uint8_t ipv4[4];
    uint8_t mac[6];
} arp_table[CONFIG_ARP_MAX_TABLE_ENTRIES] = {0};

int arp_tx_stack(struct net_socket *sock, struct net_link_layer_info *layer, struct net_ifaddr *ifaddr,
                 struct net_sock_addr addr, uint8_t ipn, void *packet, uint32_t ln) {
    return NET_DROPPED;
}

int arp_resolve_ipv4(struct net_link_layer_info *layer, struct net_ifaddr *ifaddr, uint8_t *ipv4, uint8_t *result_mac) {
    if (!layer || !ifaddr || !ipv4 || !result_mac)
        return -1;

    struct arp_resolv_ipv4_packet *resolv;
    uint32_t flags;
    int i, req = -1;
    uint64_t timeout, tries = 15;

    /* Maybe that's us? */
    if (memcmp(ifaddr->address.ipv4, ipv4, 4) == 0) {
        memcpy(result_mac, ifaddr->device->info.mac_addr, 6);
        return 0;
    }

    /* Firstly check if we cached this entry */
    for (i = 0; i < CONFIG_ARP_MAX_TABLE_ENTRIES; i++) {
        /* Check if valid entry AND hasn't expired */
        if (arp_table[i].available && arp_table[i].timeout > uptime_us()) {
            if (memcmp(arp_table[i].ipv4, ipv4, 4) == 0) {
                memcpy(result_mac, arp_table[i].mac, 6);
                return 0;
            }
        } else {
            /* Invalidate it in any other caes */
            arp_table[i].available = 0;
        }
    }

    /* Acquire buffer to receive the ARP reply */
    spin_lock_irqsave(&arp_req_lock, flags);
    for (i = 0; i < 16; i++) {
        if (!arp_req_list[i].acquired) {
            arp_req_list[i].acquired = 1;
            req = i;
            break;
        }
    }
    spin_unlock_irqrestore(&arp_req_lock, flags);

    if (req == -1) {
        return -1;
    }

try_again:
    /* Allocate buffer for data layering */
    if (layer->ops->req_buf(layer, (void **)&resolv, sizeof(*resolv)) != 0) {
        /* Unable to allocate buffer for answer */
        arp_req_list[req].acquired = 0;
        return NET_DROPPED;
    }

    resolv->hardware_type = bigend16(1); // Ethernet link protocol
    resolv->protocol_type = bigend16(NET_ETHTYPE_IPV4);
    resolv->hardware_ln = 6;
    resolv->protocol_ln = 4;
    resolv->operation = bigend16(ARP_REQ);

    memcpy(resolv->source_hardware_addr, layer->device_mac, 6);
    memcpy(&resolv->source_ipv4, ifaddr->address.ipv4, 4);

    memset(&resolv->destination_hardware_addr, 0, 6);
    memcpy(&resolv->destination_ipv4, ipv4, 4);

    /* Prepare request list entry */
    arp_req_list[req].delivered = 0;
    memcpy(&arp_req_list[req].ipv4_source, ipv4, 4);

    if (layer->ops->tx(layer, resolv, sizeof(*resolv)) != 0) {
        /* Unable to send the packet */
        arp_req_list[req].acquired = 0;
        return -1;
    }

    /* Wait for the answer
     * TODO: polling is dumb, but sufficient for now
     */
    timeout = uptime_us() + 500000; // 0.5 seconds timeout
    while (timeout > uptime_us() && !arp_req_list[i].delivered)
        sched_yield();

    /* If we got result, save it */
    if (arp_req_list[i].delivered) {
        memcpy(result_mac, arp_req_list[i].result.source_hardware_addr, 6);

        /* Store in any available table entry */
        for (i = 0; i < CONFIG_ARP_MAX_TABLE_ENTRIES; i++) {
            if (!arp_table[i].available) {
                arp_table[i].timeout = uptime_us() + 30000000; // 30 seconds expiry
                arp_table[i].available = 1;
                memcpy(arp_table[i].ipv4, ipv4, 6);
                memcpy(arp_table[i].mac, result_mac, 6);
                break;
            }
        }

        arp_req_list[i].acquired = 0;
        return 0;
    } else if (tries-- > 0) {
        goto try_again;
    }

    arp_req_list[i].acquired = 0;
    return -1;
}

int arp_rx_stack(struct net_link_layer_info *layer, void *packet, uint32_t ln) {
    struct arp_resolv_ipv4_packet *resolv = packet;
    struct arp_resolv_ipv4_packet *response;
    uint32_t flags;
    int i;

    if (bigend16(resolv->protocol_type) != NET_ETHTYPE_IPV4)
        return NET_DROPPED;

    switch (bigend16(resolv->operation)) {
    case ARP_REQ:
        /* Allocate memory for response */
        struct list_head *pos;
        struct net_ifaddr *entry;
        list_for_each(pos, &layer->dev->ifaddrs) {
            entry = list_entry(pos, struct net_ifaddr, list);
            if (memcmp(entry->address.ipv4, &resolv->destination_ipv4, 4) == 0)
                goto match;
        }

        /* Didn't match the IPv4 address */
        return NET_DROPPED;
    match:
        if (layer->ops->req_buf(layer, (void **)&response, ln) != 0)
            /* Unable to allocate buffer for answer */
            return NET_DROPPED;

        response->hardware_type = resolv->hardware_type;
        response->protocol_type = resolv->protocol_type;
        response->hardware_ln = resolv->hardware_ln;
        response->protocol_ln = resolv->protocol_ln;
        response->operation = bigend16(ARP_REPLY);

        /* Set the destination to the requester */
        memcpy(response->destination_hardware_addr, resolv->source_hardware_addr, 6);
        response->destination_ipv4 = resolv->source_ipv4;

        /* Now set the source hardware/protocol addresses */
        memcpy(response->source_hardware_addr, layer->device_mac, 6);
        memcpy(&response->source_ipv4, entry->address.ipv4, 4);

        return layer->ops->tx(layer, response, ln);
    case ARP_REPLY:
        /* Check if any list entry is waiting for this */
        spin_lock_irqsave(&arp_req_lock, flags);
        for (i = 0; i < 16; i++) {
            if (arp_req_list[i].acquired && memcmp(&resolv->source_ipv4, arp_req_list[i].ipv4_source, 4) == 0) {
                memcpy(&arp_req_list[i].result, resolv, sizeof(*resolv));
                arp_req_list[i].delivered = 1;
                spin_unlock_irqrestore(&arp_req_lock, flags);
                return 0;
            }
        }
        spin_unlock_irqrestore(&arp_req_lock, flags);

        kprintf("arp: bogus ARP reply.");
        return NET_DROPPED;
    }

    return NET_DROPPED;
}

#endif
