#ifdef CONFIG_SUBSYS_NET
#ifdef CONFIG_NET_ICMP

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/module.h>
#include <kernel/net/ipv4.h>
#include <kernel/subsys/net.h>

#include <misc/types.h>

int icmp_handler(struct ipv4_packet *packet, size_t ln) {
    uint32_t src = packet->src_address;
#ifdef CONFIG_LITTLE_ENDIAN
    src = swap_endian32(src);
#endif

    kprintf("icmp: packet from %d.%d.%d.%d", src >> 24, (src >> 16) & 0xff, (src >> 8) & 0xff, src & 0xff);

    return NET_HANDLED;
}

static int net_icmp_probe() {
    return ipv4_subscribe(0x01, &icmp_handler);
}

static void net_icmp_cleanup() {}

module_t net_icmp_module = {.probe = net_icmp_probe, .cleanup = net_icmp_cleanup};

module_register("net_icmp", net_icmp_module);


#endif
#endif
