#ifndef _INARI_NET_H
#define _INARI_NET_H

#include <misc/types.h>

struct net_ops
{
    int (*tx)(struct net_device *bdev, void *packet, uint32_t length);
    int (*ioctl)(struct net_device *bdev, unsigned long req, void *arg);
};

struct net_device
{
    char name[16];
    
    uint8_t mac_addr[6];
    uint16_t mtu;
    
    uint32_t flags;
    uint8_t link_state;      // 1 = Cable plugged in, 0 = Cable unplugged
    
    struct net_ops *ops;
    
    void *driver_data;
};

int net_init(void);
int net_register_device();
int net_unregister_device();

#endif
