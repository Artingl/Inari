#ifdef CONFIG_SUBSYS_NET

#include <kernel/inari.h>
#include <kernel/net/ipv4.h>
#include <kernel/subsys/net.h>

#include <misc/types.h>

static ipv4_handler handlers[0xff] = {0};

int ipv4_subscribe(uint8_t ipn, ipv4_handler handler) {
    if (handlers[ipn])
        return -1;
    handlers[ipn] = handler;
#ifdef CONFIG_DEBUG
    kprintf("ipv4: new subscriber 0x%x (%s)", ipn, ipnstr[ipn] ? ipnstr[ipn] : "unknown");
#endif
    return 0;
}

void ipv4_unsubscribe(uint8_t ipn) {
#ifdef CONFIG_DEBUG
    kprintf("ipv4: unsubscribed 0x%x (%s)", ipn, ipnstr[ipn] ? ipnstr[ipn] : "unknown");
#endif
    handlers[ipn] = NULL;
}

int ipv4_rx_stack(struct ethernet_frame *frame, size_t ln) {
    /* Minimum size is 64 bytes for ethernet frame + 20 byes for IPv4 packet */
    if (ln < 84) {
        return NET_DROPPED;
    }

    struct ipv4_packet *packet = (struct ipv4_packet *)&frame->data;

    /* Send the packet to correct protocol handler */
    if (handlers[packet->protocol])
        return handlers[packet->protocol](packet, ln - sizeof(struct ethernet_frame));
    return NET_DROPPED;
}

#endif
