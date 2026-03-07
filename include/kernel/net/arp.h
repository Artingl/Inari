#ifndef _INARI_NET_ARP
#define _INARI_NET_ARP

#include <kernel/subsys/net.h>
#include <misc/types.h>

int arp_rx_stack(struct ethernet_frame *frame, size_t ln);

#endif
