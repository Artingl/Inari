#ifdef CONFIG_SUBSYS_NET
#ifdef CONFIG_NET_UDP

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/module.h>
#include <kernel/net/ipv4.h>
#include <kernel/subsys/net.h>

#include <misc/string.h>
#include <misc/types.h>

struct udp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t ln;
    uint16_t checksum;
    uint8_t data[];
} __attribute__((packed));

struct udp_header_checksum {
    struct {
        uint32_t src_address;
        uint32_t dest_address;
        uint8_t zeroes;
        uint8_t protocol;
        uint16_t udp_ln;
    } ipv4_pseudo;

    struct udp_header actual;
} __attribute__((packed));

/* TODO: ??? MVP */
static pid_t ports[65535] = {0};

int udp_rx_handler(struct net_network_layer_info *layer, struct udp_header *packet, uint32_t ln) {
    // uint16_t checksum = bigend16(packet->checksum);
    // packet->checksum = 0;
    /* I dont care for now */
    // if (net_checksum(packet, ln) != checksum) {
    //     kprintf("udp bad checksum");
    //     return NET_DROPPED;
    // }

    /* TODO: UDP is datagram protocol, packets must not leak */
    return -1;//net_sock_fill_datagram(bigend16(packet->dest_port), packet->data, ln - sizeof(struct udp_header));
}

static int udp_tx_handler(struct net_socket *sock, struct net_sock_addr addr, struct net_network_layer_info *layer,
                          void *packet, uint32_t ln) {
    struct udp_header_checksum *udp;

    /* Allocate buffer for data layering */
    if (layer->ops->req_buf(layer, (void **)&udp, ln + sizeof(struct udp_header_checksum)) != 0)
        /* Unable to allocate buffer for answer */
        return NET_DROPPED;

    /* Setup pseudo IPv4 header */
    memcpy(&udp->ipv4_pseudo.dest_address, addr.address, 4);
    memcpy(&udp->ipv4_pseudo.src_address, layer->ifaddr->address.ipv4, 4);
    udp->ipv4_pseudo.zeroes = 0;
    udp->ipv4_pseudo.protocol = NET_IPN_UDP;
    udp->ipv4_pseudo.udp_ln = bigend16(ln + sizeof(struct udp_header));

    /* Set the identifier so we later dont lose the packet */
    memcpy(udp->actual.data, packet, ln);
    udp->actual.dest_port = bigend16(addr.identifier);
    udp->actual.src_port = bigend16(sock->identifier);
    udp->actual.ln = bigend16(ln + sizeof(struct udp_header));
    udp->actual.checksum = 0;
    udp->actual.checksum = bigend16(net_checksum(udp, ln + sizeof(struct udp_header_checksum)));

    /* Transmit data */
    memmove(udp, &udp->actual, ln + sizeof(struct udp_header));
    return layer->ops->tx(layer, udp, ln + sizeof(struct udp_header));
}

static int udp_connect(struct net_socket *sock, struct net_sock_addr addr) {
    /* Allocate port that will listen for packets.
     * TODO: again, dumb;
     * TODOx2: connect in UDP is strange, we can move this logic to TX */
    for (size_t i = 65535; i > 0; i--) {
        if (!ports[i]) {
            ports[i] = sock->owner;
            sock->identifier = i;
            return 0;
        }
    }

    return NET_ERR;
}

static int udp_bind(struct net_socket *sock, struct net_sock_addr addr) {
    if (ports[addr.identifier])
        return -EBUSY;
    ports[addr.identifier] = sock->owner;
    sock->identifier = addr.identifier;
    return 0;
}

static int udp_close(struct net_socket *sock) {
    if (ports[sock->identifier] != sock->owner)
        return -EPERM;
    ports[sock->identifier] = 0;
    return 0;
}

static int net_udp_probe() {
    return net_define_protocol(NET_IPN_UDP, (struct net_protocol){.is_privileged = 0,
                                                                  .rx = (net_protocol_rx_t)&udp_rx_handler,
                                                                  .tx = (net_protocol_tx_t)&udp_tx_handler,
                                                                  .bind = (net_protocol_transport_t)&udp_bind,
                                                                  .connect = (net_protocol_transport_t)&udp_connect,
                                                                  .close = (net_protocol_close_t)&udp_close});
}

static void net_udp_cleanup() { net_free_protocol(NET_IPN_UDP); }

module_t net_udp_module = {.probe = net_udp_probe, .cleanup = net_udp_cleanup};

module_register("net_udp", net_udp_module);

#endif
#endif
