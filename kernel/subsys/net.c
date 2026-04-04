#ifdef CONFIG_SUBSYS_NET

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/vmm.h>
#include <kernel/net/arp.h>
#include <kernel/net/ipv4.h>
#include <kernel/proc/sched.h>
#include <kernel/subsys/net.h>
#include <kernel/sync/spinlock.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>
#include <kernel/sys/vfs.h>

#include <misc/format.h>
#include <misc/list.h>
#include <misc/ring.h>
#include <misc/string.h>
#include <misc/types.h>

#define RING_LN 512

static struct net_frame net_ring_buffer[RING_LN] = {0};

static spinlock_t net_handles_lock = {0};
static net_handle_t net_handles_last = 0xff;

static LIST_HEAD(net_handles);
static LIST_HEAD(net_routing_table);
static LIST_HEAD(net_devices);

static int device_id = 0;
static int is_initialized = 0;
static tid_t queue_thread;
static struct net_protocol protocols[0xff];

static struct net_buf net_buf_pool[CONFIG_MAX_NET_BUFFERS];
static uint16_t free_pool_entries[CONFIG_MAX_NET_BUFFERS];
static uint16_t free_pool_index = 0;

static struct net_device *get_device(dev_t dev) {
    struct list_head *pos;
    struct net_device *entry;

    list_for_each(pos, &net_devices) {
        entry = list_entry(pos, struct net_device, list);
        if (entry->dev == dev)
            return entry;
    }

    return NULL;
}

static struct net_device *get_device_name(const char *name) {
    struct list_head *pos;
    struct net_device *entry;

    list_for_each(pos, &net_devices) {
        entry = list_entry(pos, struct net_device, list);
        if (strcmp(entry->info.name, name) == 0)
            return entry;
    }

    return NULL;
}

static int count_bits(unsigned int n) {
    int count = 0;
    while (n > 0) {
        n &= (n - 1); // Clears the least significant set bit
        count++;
    }
    return count;
}

static struct net_ifaddr *get_ifaddr_route(struct net_device *target, uint8_t *addr, size_t sz, uint8_t *is_default) {
    struct list_head *pos;
    struct net_route_entry *route_entry;
    uint32_t in_addr = *((uint32_t *)addr), subnet_mask, dest_network;

    /* TODO: support other than ipv4 */
    if (sz != 4)
        return NULL;

    uint8_t best_mask = 0;
    uint8_t best_default = 0;
    struct net_ifaddr *best_ifaddr = NULL;

    list_for_each(pos, &net_routing_table) {
        route_entry = list_entry(pos, struct net_route_entry, list);
        subnet_mask = *((uint32_t *)route_entry->netmask);
        dest_network = *((uint32_t *)route_entry->dest_network);
        if ((in_addr & subnet_mask) == dest_network && ((target && route_entry->ifaddr->device == target) || !target)) {
            if (best_mask <= count_bits(bigend32(subnet_mask))) {
                best_mask = count_bits(bigend32(subnet_mask));
                best_default = route_entry->is_default;
                best_ifaddr = route_entry->ifaddr;
            }
        }
    }

    if (best_ifaddr) {
        if (is_default)
            *is_default = best_default;
        return best_ifaddr;
    }

    return NULL;
}

static void cleanup_routing(struct net_device *dev) {
    struct list_head *pos, *n;
    struct net_route_entry *entry;

    list_for_each_safe(pos, n, &net_routing_table) {
        entry = list_entry(pos, struct net_route_entry, list);
        if (entry->ifaddr->device == dev) {
            list_del(pos);
            kfree(entry);
        }
    }
}

static void update_routing(struct net_device *dev) {
    /* Ensure we don't duplicate routing entries */
    cleanup_routing(dev);

    struct list_head *pos;
    struct net_route_entry *route_entry;
    struct net_ifaddr *entry;
    uint32_t dev_addr, dev_mask, res;

    list_for_each(pos, &dev->ifaddrs) {
        entry = list_entry(pos, struct net_ifaddr, list);

        /* Local link */
        route_entry = kmalloc(sizeof(struct net_route_entry));

        /* OOM */
        if (!route_entry)
            return;

        dev_addr = *((uint32_t *)entry->address.ipv4);
        dev_mask = *((uint32_t *)entry->netmask.ipv4);

        res = dev_addr & dev_mask;
        memcpy(route_entry->dest_network, &res, 4);
        memcpy(route_entry->netmask, entry->netmask.ipv4, 4);
        memset(route_entry->gateway, 0, 4);
        route_entry->ifaddr = entry;
        route_entry->is_default = 0;
        list_add(&route_entry->list, &net_routing_table);

        /* Add default gateway if ifaddr has one */
        if (*((uint32_t *)entry->gateway.ipv4) != 0) {
            route_entry = kmalloc(sizeof(struct net_route_entry));

            /* OOM */
            if (!route_entry)
                return;

            memset(route_entry->dest_network, 0, 4);
            memset(route_entry->netmask, 0, 4);
            memcpy(route_entry->gateway, entry->gateway.ipv4, 4);
            route_entry->ifaddr = entry;
            route_entry->is_default = 1;
            list_add(&route_entry->list, &net_routing_table);
        }
    }
}

static struct net_socket *get_sock_handle(net_handle_t handle) {
    struct list_head *pos;
    struct net_socket *entry;

    list_for_each(pos, &net_handles) {
        entry = list_entry(pos, struct net_socket, list);
        if (entry->handle == handle)
            return entry;
    }

    return NULL;
}

static struct net_socket *get_sock_identifier(uint32_t identifier) {
    struct list_head *pos;
    struct net_socket *entry;

    list_for_each(pos, &net_handles) {
        entry = list_entry(pos, struct net_socket, list);
        if (entry->identifier == identifier)
            return entry;
    }

    return NULL;
}

static int cleanup_sock(struct net_socket *sock) {
    list_del(&sock->list);

    struct net_protocol *protocol = net_invoke_protocol(sock->ipn);
    if (protocol && protocol->close)
        protocol->close(sock);
    /* TODO: smarter logic here. */

    kfree(sock);
    return 0;
}

static int sock_create(struct net_socket **sock, net_handle_t *handle) {
    *handle = net_handles_last++;
    *sock = kmalloc(sizeof(struct net_socket));
    if (!*sock)
        return -ENOMEM;
    memset(*sock, 0, sizeof(**sock));
    (*sock)->handle = *handle;
    list_add(&(*sock)->list, &net_handles);
    return 0;
}

static int cleanup_sock_handle(net_handle_t handle) {
    struct net_socket *sock;
    if (!(sock = get_sock_handle(handle)))
        return -1;
    return cleanup_sock(sock);
}

int net_link_layer_tx(void *layer_info, void *packet, uint32_t ln) {
    if (!packet)
        return -1;

    /* Construct ethernet frame and transmit */
    struct net_link_layer_info *info = layer_info;

    /* Step back in packet buffer to access memory allocated for ethernet frame by `req_buf` */
    struct ethernet_frame *frame = (struct ethernet_frame *)(packet - sizeof(struct ethernet_frame));

    memcpy(frame->src_mac, info->device_mac, 6);
    memcpy(frame->dest_mac, info->src_mac, 6);
    frame->ether_type = info->layer->ether_type;

    /* Finally, send packet */
    int res = net_tx_packet(info->dev->dev, frame, ln + sizeof(struct ethernet_frame));

    /* Don't forget to deallocate buffer! */
    kfree(frame);
    return res;
}

int net_link_layer_req_buf(void *layer_info, void **result, uint32_t ln) {
    if (!result && !*result) // struct net_buf
        return -EINVAL;
    if ((*result = kmalloc((ln + sizeof(struct ethernet_frame)))) == NULL)
        return -ENOMEM;
    *result += sizeof(struct ethernet_frame);
    return 0;
}

static struct net_layer_ops net_link_layer_ops = {
    .tx = &net_link_layer_tx,
    .req_buf = &net_link_layer_req_buf,
};

static void net_queue_thread(void *_) {
    while (!is_initialized)
        ;

    struct net_frame *net_frame = NULL;
    struct ethernet_frame *frame;
    struct net_link_layer_info link;
    struct net_device *device;
    size_t i = 0;
    int res;
    do {
        /* Loop to find waiting frames in queue */
        while (!(net_frame = &net_ring_buffer[i++ % RING_LN])->set)
            sched_yield();

        /* Minimum size of ethernet frame is 64 bytes */
        if (net_frame->length < 64) {
            res = NET_DROPPED;
            goto end;
        }

        /* Verify we have such device */
        if (!(device = get_device(net_frame->dev))) {
            res = NET_DROPPED;
            goto end;
        }

        frame = net_frame->data;

        /* Create link layer representation for next layers */
        memcpy(link.src_mac, frame->src_mac, 6);
        memcpy(link.dest_mac, frame->dest_mac, 6);
        memcpy(link.device_mac, device->info.mac_addr, 6);
        link.layer = frame;
        link.dev = device;
        link.ops = &net_link_layer_ops;
        switch (bigend16(frame->ether_type)) {
        case NET_ETHTYPE_IPV4:
            res = ipv4_rx_stack(&link, (struct ipv4_packet *)frame->data,
                                net_frame->length - sizeof(struct ethernet_frame));
            break;
        case NET_ETHTYPE_ARP:
            res = arp_rx_stack(&link, frame->data, net_frame->length - sizeof(struct ethernet_frame));
            break;
        default:
            res = NET_DROPPED;
        }
    end:
        if (res == NET_DROPPED) {
#ifdef CONFIG_DEBUG
            kprintf("net: dropped rx 0x%x, ln 0x%x; ether type: 0x%x", net_frame->data, net_frame->length,
                    swap_endian16(frame->ether_type));
#endif
        }

        /* Before completing, write down 64 bytes of zeros onto data pointer. This ensures we obliterate the ethernet
         * frame and dont accidentally handle it again. TODO: that's actually bad and we dont even need that in the
         * first place.
         *
         * The issue is that the data pointer is currently passed from NICs ring buffer. We need to copy it to the net
         * subsystem ring buffer, so we dont run into race conditions or other issues by directly accessing NICs ring
         * buffer. */
        memset(net_frame->data, 0, 64);

        net_frame->set = 0;
        net_frame->data = NULL;
        net_frame->length = 0;

        /* Don't forget to unlock, so net_rx can use this frame spot */
        spin_unlock(&net_frame->frame_lock);
    } while (1);
}

static int net_ioctl(struct device *chardev, unsigned long req, void *arg) {
    if (!chardev || !chardev->driver_data)
        return -EINVAL;
    struct net_device *device = (struct net_device *)chardev->driver_data;
    struct list_head *pos;
    struct net_ifaddr *entry, *ifaddr;
    int found = -1, idx = 0;

    switch (req) {
    case NET_IOCTL_INFO:
        if (VMM_IS_PTR_USERSPACE(arg) && VMM_IS_PTR_USERSPACE(arg + sizeof(device->info)))
            memcpy(arg, &device->info, sizeof(device->info));
        return 0;
    case NET_IOCTL_ATTACH_IFADDR:
        if (VMM_IS_PTR_USERSPACE(arg) && VMM_IS_PTR_USERSPACE(arg + sizeof(struct net_ifaddr)))
            return net_attach_ifaddr(device->dev, *((struct net_ifaddr *)arg));
        return -EINVAL;
    case NET_IOCTL_DETACH_IFADDR:
        if (VMM_IS_PTR_USERSPACE(arg) && VMM_IS_PTR_USERSPACE(arg + sizeof(struct net_ifaddr)))
            return net_detach_ifaddr(device->dev, *((struct net_ifaddr *)arg));
        return -EINVAL;
    case NET_IOCTL_IFADDR_NEXT:
        if (VMM_IS_PTR_USERSPACE(arg) && VMM_IS_PTR_USERSPACE(arg + sizeof(struct net_ifaddr))) {
            ifaddr = arg;
            if (!ifaddr->device)
                found = 0;

            list_for_each(pos, &device->ifaddrs) {
                entry = list_entry(pos, struct net_ifaddr, list);
                if (idx++ == found) {
                    memcpy(arg, entry, sizeof(struct net_ifaddr));
                    return 1;
                }

                if (memcmp(entry->address.ipv4, ifaddr->address.ipv4, sizeof(ifaddr->address.ipv4)) == 0 ||
                    memcmp(entry->address.ipv6, ifaddr->address.ipv6, sizeof(ifaddr->address.ipv6)) == 0) {
                    found = idx;
                }
            }
        }
        return 0;
    }

    return -ENOSYS;
}

static struct char_ops ops = {.ioctl = &net_ioctl};

struct net_protocol *net_invoke_protocol(uint8_t ipn) {
    if (!protocols[ipn].rx)
        return NULL;
    return &protocols[ipn];
}

int net_buf_alloc(struct net_buf **result) {
    if (!result)
        return -EINVAL;
    if (free_pool_index == 0)
        return -ENOMEM;

    free_pool_index--;
    uint16_t index = free_pool_entries[free_pool_index];
    *result = &net_buf_pool[index];

    return 0;
}

int net_buf_free(struct net_buf *buf) {
    if (!buf)
        return -EINVAL;

    if (free_pool_index >= CONFIG_MAX_NET_BUFFERS) {
        kprintf("net: invalid net_buf stack entry.");
        return -EINVAL;
    }
    /* Clear the buffer */
    memset(&buf->origin, 0, sizeof(buf->origin));
    buf->ln = 0;

    /* Free it */
    uint16_t index = buf - net_buf_pool;
    free_pool_entries[free_pool_index] = index;
    free_pool_index++;
    return 0;
}

int net_init(void) {
    for (size_t i = 0; i < CONFIG_MAX_NET_BUFFERS; i++) {
        free_pool_entries[i] = i;
    }
    free_pool_index = CONFIG_MAX_NET_BUFFERS;

    memset(&protocols, 0, sizeof(protocols));
    register_chardev_group(NET_DRIVER, "net");
    is_initialized = 1;
    return sched_create_thread("net_queue", &queue_thread, &net_queue_thread, NULL, NULL, NULL);
}

int net_define_protocol(uint8_t ipn, struct net_protocol protocol) {
    if (protocols[ipn].rx)
        return -1;
    protocols[ipn] = protocol;
    return 0;
}

void net_free_protocol(uint8_t ipn) { memset(&protocols[ipn], 0, sizeof(struct net_protocol)); }

void net_proc_cleanup(struct process *proc) {
    uint32_t flags;
    spin_lock_irqsave(&net_handles_lock, flags);

    struct list_head *pos, *n;
    struct net_socket *entry;

    list_for_each_safe(pos, n, &net_handles) {
        entry = list_entry(pos, struct net_socket, list);
        if (entry->owner == proc->pid)
            cleanup_sock(entry);
    }

    spin_unlock_irqrestore(&net_handles_lock, flags);
}

int net_syscall(pid_t caller, struct net_sys_command *command, net_handle_t *sock_handle) {
    /* Crucial note! When adding new commands, don't forget to check if pointers/buffers ARE IN USERSPACE
     * The `command` and `sock_handle` structure pointers are already checked by the syscall handler.
     */
    int ret = -ENOSYS;
    uint16_t ln;
    uint64_t timeout;
    uint32_t flags;
    uint16_t ethtype;
    uint8_t is_default;
    uint8_t dest_mac[6];
    uint8_t origin_ipv4[4];
    struct net_buf *next_buf;
    struct net_socket *sock;
    struct net_ifaddr *ifaddr;
    struct net_device *device = NULL;
    struct ethernet_frame frame;
    struct net_link_layer_info link;
    struct net_protocol *protocol;
    spin_lock_irqsave(&net_handles_lock, flags);

    switch (command->id) {
    case NET_SYS_CREATE:
        /* Ensure data */
        if (!sock_handle) {
            ret = -EINVAL;
            goto end;
        }

        /* Check that the request IPN exists (and in future that we have sufficient permissions) */
        if (!net_invoke_protocol(command->as.create.ipn)) {
            ret = -ENOSYS;
            goto end;
        }

        /* Check that a valid device is provided */
        if (command->as.create.net_device[0]) {
            if (!(device = get_device_name(command->as.create.net_device))) {
                ret = -ENODEV;
                goto end;
            }
        }

        /* Finally, create the socket */
        if ((ret = sock_create(&sock, sock_handle)) != 0)
            goto end;
        sock->ethtype = command->as.create.ethtype;
        sock->ipn = command->as.create.ipn;
        sock->owner = caller;
        sock->dev = device;
        ret = 0;
        break;

    case NET_SYS_FREE:
        /* Ensure data */
        if (!sock_handle) {
            ret = -EINVAL;
            goto end;
        }

        ret = cleanup_sock_handle(*sock_handle);
        break;
    case NET_SYS_SENDTO:
        /* Ensure data */
        if (!VMM_IS_RANGE_USERSPACE(command->as.flow.buffer, command->as.flow.buffer + command->as.flow.buffer_sz) ||
            !sock_handle) {
            ret = -EINVAL;
            goto end;
        }

        if (!(sock = get_sock_handle(*sock_handle))) {
            ret = -EINVAL;
            goto end;
        }
        ethtype = sock->ethtype;

        /* Ensure we're the owner */
        if (sock->owner != caller) {
            ret = -EINVAL;
            goto end;
        }

        /* TODO: only ipv4 support for now */
        if (command->as.flow.addr.addr_ln != 4) {
            ret = -EINVAL;
            goto end;
        }

        /* Find required ifaddr for our destination. */
        if (!(ifaddr = get_ifaddr_route(sock->dev, command->as.flow.addr.address, command->as.flow.addr.addr_ln,
                                        &is_default))) {
            ret = -ENETUNREACH;
            goto end;
        }

        memcpy(origin_ipv4, ifaddr->address.ipv4, 4);

        /* We dont need a lock anymore. Also we need to unlock here to avoid hard-locks. */
        spin_unlock_irqrestore(&net_handles_lock, flags);

        /* Construct ARP frame */
        link.dev = ifaddr->device;
        link.ops = &net_link_layer_ops;
        link.layer = &frame;
        frame.ether_type = bigend16(NET_ETHTYPE_ARP);
        memset(link.src_mac, 0xff, 6);
        memset(link.dest_mac, 0xff, 6);
        memcpy(link.device_mac, ifaddr->device->info.mac_addr, 6);

        /* Note: we know for sure that it is IPv4 in flow.addr because of condition above. */
        /* Note: we will force MAC ff:ff:ff:ff:ff:ff for destination address 255.255.255.255 */
        if ((ifaddr->gateway.ipv4[0] == 0xff && ifaddr->gateway.ipv4[1] == 0xff && ifaddr->gateway.ipv4[2] == 0xff &&
             ifaddr->gateway.ipv4[3] == 0xff) ||
            (command->as.flow.addr.address[0] == 0xff && command->as.flow.addr.address[1] == 0xff &&
             command->as.flow.addr.address[2] == 0xff && command->as.flow.addr.address[3] == 0xff)) {
            memset(dest_mac, 0xff, 6);
        } else if (is_default) {
            if (arp_resolve_ipv4(&link, ifaddr, ifaddr->gateway.ipv4, dest_mac) != 0) {
                ret = -ENETUNREACH;
                goto end;
            }
        } else {
            if (arp_resolve_ipv4(&link, ifaddr, command->as.flow.addr.address, dest_mac) != 0) {
                ret = -ENETUNREACH;
                goto end;
            }
        }

        /* Construct correct frame */
        /* Note: here we set both values to the dest MAC address, the net TX
         * will handle all MAC addresses on its own */
        frame.ether_type = bigend16(ethtype);
        memcpy(link.src_mac, dest_mac, 6);
        memcpy(link.dest_mac, dest_mac, 6);

        /* Lock again and check that socket is still alive while we were waiting for ARP resolve */
        spin_lock_irqsave(&net_handles_lock, flags);
        if (!(sock = get_sock_handle(*sock_handle))) {
            ret = -ENETDOWN;
            goto end;
        }

        switch (ethtype) {
        case NET_ETHTYPE_IPV4:
            ret = ipv4_tx_stack(sock, &link, origin_ipv4, command->as.flow.addr, sock->ipn, command->as.flow.buffer,
                                command->as.flow.buffer_sz);
            break;
        case NET_ETHTYPE_ARP:
            ret = arp_tx_stack(sock, &link, origin_ipv4, command->as.flow.addr, sock->ipn, command->as.flow.buffer,
                               command->as.flow.buffer_sz);
            break;
        default:
            ret = NET_DROPPED;
        }

        ret = 0;
        break;
    case NET_SYS_RECVFROM:
        /* Ensure data */
        if (!VMM_IS_RANGE_USERSPACE(command->as.flow.buffer, command->as.flow.buffer + command->as.flow.buffer_sz) ||
            !VMM_IS_RANGE_USERSPACE(command->as.flow.from, command->as.flow.from + sizeof(struct net_sock_addr)) ||
            !sock_handle) {
            ret = -EINVAL;
            goto end;
        }

        if (!(sock = get_sock_handle(*sock_handle))) {
            ret = -EINVAL;
            goto end;
        }

        /* Ensure we're the owner */
        if (sock->owner != caller) {
            ret = -EINVAL;
            goto end;
        }

        /* We dont need a lock anymore. Also we need to unlock here to avoid hard-locks. */
        spin_unlock_irqrestore(&net_handles_lock, flags);

        /* TODO: semaphore */
        if (command->as.flow.timeout_us > 0) {
            timeout = uptime_us() + command->as.flow.timeout_us;
            while (((sock->is_stream && sock->rx.stream.head == sock->rx.stream.tail) ||
                    (!sock->is_stream && !sock->rx.datagram.buf)) &&
                   timeout > uptime_us())
                sched_yield();
        } else {
            while (((sock->is_stream && sock->rx.stream.head == sock->rx.stream.tail) ||
                    (!sock->is_stream && !sock->rx.datagram.buf)))
                sched_yield();
        }

        if (timeout <= uptime_us()) {
            ret = -ETIMEDOUT;
            goto end;
        }

        /* Now extract the first arrived packet */
        if (sock->is_stream) {
            ln = (sock->rx.stream.tail - sock->rx.stream.head + 65535) % 65535;
            if (command->as.flow.buffer_sz < ln)
                ln = command->as.flow.buffer_sz;

            /* Fill the full buffer */
            if ((uint32_t)(65535 - sock->rx.stream.tail) > ln) {
                memcpy(command->as.flow.buffer, &sock->rx.stream.ring_buffer[sock->rx.stream.tail], ln);
                sock->rx.stream.tail += ln;
            }
            /* Fill in chunks */
            else {
                memcpy(command->as.flow.buffer, &sock->rx.stream.ring_buffer[sock->rx.stream.tail],
                       sock->rx.stream.tail - ln);
                memcpy(command->as.flow.buffer + (sock->rx.stream.tail - ln), &sock->rx.stream.ring_buffer[0],
                       ln - (sock->rx.stream.tail - ln));
                sock->rx.stream.tail = ln - (sock->rx.stream.tail - ln);
            }

            /* Save the origin address */
            if (command->as.flow.from)
                *command->as.flow.from = sock->rx.stream.origin;
            ret = ln;
        } else {
            /* Lock so we are safely working with the buf */
            spin_lock_irqsave(&net_handles_lock, flags);
            if (!sock->rx.datagram.buf) {
                ret = -EINVAL;
                goto end;
            }

            /* TODO: Allow packets to be larger than single MTU */
            ln = MIN(sock->rx.datagram.buf->ln, MIN(command->as.flow.buffer_sz, 1500));
            memcpy(command->as.flow.buffer, sock->rx.datagram.buf->payload, ln);

            /* Save the origin address */
            if (command->as.flow.from)
                *command->as.flow.from = sock->rx.datagram.buf->origin;

            next_buf = list_entry(sock->rx.datagram.buf->list.next, struct net_buf, list);
            list_del(&sock->rx.datagram.buf->list);

            net_buf_free(sock->rx.datagram.buf);
            if (sock->rx.datagram.buf == next_buf)
                sock->rx.datagram.buf = 0;
            else
                sock->rx.datagram.buf = next_buf;
            ret = ln;
        }

        break;
    case NET_SYS_BIND:
        /* Ensure data */
        if (!sock_handle) {
            ret = -EINVAL;
            goto end;
        }

        if (!(sock = get_sock_handle(*sock_handle))) {
            ret = -EINVAL;
            goto end;
        }

        /* Ensure we're the owner */
        if (sock->owner != caller) {
            ret = -EINVAL;
            goto end;
        }

        protocol = net_invoke_protocol(sock->ipn);
        ret = -ENOSYS;
        if (protocol && protocol->bind)
            ret = protocol->bind(sock, command->as.transport.addr);

        break;
    case NET_SYS_CONNECT:
        /* Ensure data */
        if (!sock_handle) {
            ret = -EINVAL;
            goto end;
        }

        if (!(sock = get_sock_handle(*sock_handle))) {
            ret = -EINVAL;
            goto end;
        }

        /* Ensure we're the owner */
        if (sock->owner != caller) {
            ret = -EINVAL;
            goto end;
        }

        protocol = net_invoke_protocol(sock->ipn);
        ret = -ENOSYS;
        if (protocol && protocol->connect)
            ret = protocol->connect(sock, command->as.transport.addr);
        break;
    case NET_SYS_REQ_NIC_INFO:
        /* Ensure data */
        if (!VMM_IS_RANGE_USERSPACE(command->as.nic_info.result,
                                    command->as.nic_info.result + sizeof(struct net_device_info))) {
            ret = -EINVAL;
            goto end;
        }

        /* Retrieve the device */
        if (!(device = get_device_name(command->as.nic_info.net_device))) {
            ret = -ENODEV;
            goto end;
        }

        if (command->as.nic_info.result) {
            memcpy(command->as.nic_info.result, &device->info, sizeof(device->info));
        }

        ret = 0;
        break;
    }

end:
    spin_unlock_irqrestore(&net_handles_lock, flags);
    return ret;
}

int net_sock_fill_stream(struct net_sock_addr origin, uint16_t dest_identifier, void *packet, uint32_t ln) {
    int ret = 0;
    uint32_t flags;
    uint16_t ring_space;
    struct net_socket *sock;
    spin_lock_irqsave(&net_handles_lock, flags);

    if (!(sock = get_sock_identifier(dest_identifier))) {
        ret = -EINVAL;
        goto end;
    }

    /* Fill the ring buffer */
    ring_space = 65535 - ((sock->rx.stream.tail - sock->rx.stream.head + 65535) % 65535);

    /* Packet doesn't fit */
    if (ring_space < ln) {
        ret = NET_DROPPED;
        goto end;
    }

    sock->rx.stream.origin = origin;
    sock->is_stream = 1;

    /* Fill the full buffer */
    if ((uint32_t)(65535 - sock->rx.stream.head) > ln) {
        memcpy(&sock->rx.stream.ring_buffer[sock->rx.stream.head], packet, ln);
        sock->rx.stream.head += ln;
    }
    /* Fill in chunks */
    else {
        memcpy(&sock->rx.stream.ring_buffer[sock->rx.stream.head], packet, sock->rx.stream.head - ln);
        memcpy(&sock->rx.stream.ring_buffer[0], packet + (sock->rx.stream.head - ln), ln - (sock->rx.stream.head - ln));
        sock->rx.stream.head = ln - (sock->rx.stream.head - ln);
    }

end:
    spin_unlock_irqrestore(&net_handles_lock, flags);
    return ret;
}

int net_sock_fill_datagram(struct net_sock_addr origin, uint16_t dest_identifier, void *packet, uint32_t ln) {
    int ret = 0;
    uint32_t flags;
    struct net_socket *sock;
    struct net_buf *buf;
    spin_lock_irqsave(&net_handles_lock, flags);

    if (!(sock = get_sock_identifier(dest_identifier))) {
        ret = -EINVAL;
        goto end;
    }

    if ((ret = net_buf_alloc(&buf)) != 0)
        goto end;

    if (!sock->rx.datagram.buf) {
        sock->rx.datagram.buf = buf;
        INIT_LIST_HEAD(&sock->rx.datagram.buf->list);
    } else {
        list_add_tail(&buf->list, &sock->rx.datagram.buf->list);
    }

    sock->rx.datagram.buf->origin = origin;
    sock->is_stream = 0;

    /* TODO: split packet if larger than MTU */
    ln = MIN(ln, 1500);
    sock->rx.datagram.buf->ln = ln;
    memcpy(sock->rx.datagram.buf->payload, packet, ln);
end:
    spin_unlock_irqrestore(&net_handles_lock, flags);
    return ret;
}

int net_add_device(dev_t *dev, struct net_ops *net_ops, uint8_t *mac, uint16_t mtu, const char *name) {
    if (!net_ops || !name || !is_initialized || !mac)
        return -EINVAL;

    int res;
    struct net_device *device = (struct net_device *)kmalloc(sizeof(struct net_device));
    if (!device)
        return -ENOMEM;

    sprintf(device->info.name, "net%d", device_id++);
    strcpy(device->info.hardware, name);
    memcpy(device->info.mac_addr, mac, sizeof(device->info.mac_addr));
    device->info.name[DEV_NAME_SIZE] = '\0';
    device->info.mtu = mtu;
    device->ops = net_ops;
    INIT_LIST_HEAD(&device->ifaddrs);

    if ((res = register_chardev(NET_DRIVER, &ops, device, &device->dev)) != 0) {
        kfree(device);
        return res;
    }

    list_add(&device->list, &net_devices);
    update_routing(device);
    if (dev)
        *dev = device->dev;

    kprintf("net: %s initialized; MAC %2x:%2x:%2x:%2x:%2x:%2x", name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return 0;
}

int net_attach_ifaddr(dev_t dev, struct net_ifaddr ifaddr) {
    struct net_device *device = get_device(dev);
    if (!device)
        return -ENODEV;

    struct list_head *pos;
    struct net_ifaddr *entry;

    /* Check we don't already have that exact address in the NIC */
    list_for_each(pos, &device->ifaddrs) {
        entry = list_entry(pos, struct net_ifaddr, list);
        if (memcmp(entry->address.ipv4, ifaddr.address.ipv4, sizeof(ifaddr.address.ipv4)) == 0 ||
            memcmp(entry->address.ipv6, ifaddr.address.ipv6, sizeof(ifaddr.address.ipv6)) == 0) {
            /* Ignore duplicate */
            return -EINVAL;
        }
    }

    /* TODO: also would be great to check that other NICs doesn't have that ifaddr */

    /* Now, attach it */
    struct net_ifaddr *list_ifaddr = kmalloc(sizeof(struct net_ifaddr));
    memcpy(list_ifaddr, &ifaddr, sizeof(ifaddr));

    list_ifaddr->device = device;
    list_add(&list_ifaddr->list, &device->ifaddrs);

    /* Don't forget to update routing table */
    update_routing(device);

    return 0;
}

int net_detach_ifaddr(dev_t dev, struct net_ifaddr ifaddr) {
    struct net_device *device = get_device(dev);
    if (!device)
        return -ENODEV;

    struct list_head *pos;
    struct net_ifaddr *entry;

    list_for_each(pos, &device->ifaddrs) {
        entry = list_entry(pos, struct net_ifaddr, list);
        if (memcmp(entry->address.ipv4, ifaddr.address.ipv4, sizeof(ifaddr.address.ipv4)) == 0 ||
            memcmp(entry->address.ipv6, ifaddr.address.ipv6, sizeof(ifaddr.address.ipv6)) == 0) {
            list_del(pos);
            kfree(entry);
            /* Don't forget to update routing table */
            update_routing(device);
            return 0;
        }
    }

    return -EINVAL;
}

int net_remove_device(dev_t dev) {
    if (!is_initialized)
        return -EINVAL;
    struct device *chardev = char_get(dev);
    if (!chardev)
        return -ENODEV;
    if (chardev->driver_data) {
        /* Don't forget to deallocate ifaddrs */
        struct list_head *pos, *n;
        struct net_ifaddr *entry;
        struct net_device *device = chardev->driver_data;

        list_for_each_safe(pos, n, &device->ifaddrs) {
            entry = list_entry(pos, struct net_ifaddr, list);
            list_del(pos);
            kfree(entry);
        }

        cleanup_routing(device);
        list_del(&device->list);
        kfree(chardev->driver_data);
    }
    return unregister_chardev(dev);
}

int net_is_active(dev_t dev) { return 1; }

int net_tx_packet(dev_t dev, void *packet, uint32_t length) {
    struct list_head *pos;
    struct net_device *entry;

    list_for_each(pos, &net_devices) {
        entry = list_entry(pos, struct net_device, list);
        if (entry->dev == dev) {
            if (entry->ops->tx(entry, packet, length) != 0)
                return NET_DROPPED;
            return NET_OK;
        }
    }

    return -ENODEV;
}

int net_rx_packet(dev_t dev, void *data, uint32_t length) {
    static size_t ring_off = 0;
    struct net_frame *frame = NULL;
    int timeout = RING_LN * 2;
    /* Find free frame to save the packet into */
    do {
        if (spin_try_lock(&net_ring_buffer[ring_off].frame_lock)) {
            frame = &net_ring_buffer[ring_off];
            ring_off = (ring_off + 1) % RING_LN;

            /* Ignore unlocked frame spot if already in use */
            if (frame->set) {
                spin_unlock(&frame->frame_lock);
                frame = NULL;
                continue;
            }

            /* Use this frame in buffer */
            break;
        }

        /* Try next spot */
        ring_off = (ring_off + 1) % RING_LN;
    } while (timeout-- > 0);

    if (!frame) {
#ifdef CONFIG_DEBUG
        kprintf("net: drop packet 0x%x, ln 0x%x", data, length);
#endif
        /* Drop frame */
        return -1;
    }

    frame->set = 1;
    frame->data = data;
    frame->length = length;
    frame->dev = dev;

    /* Keep locked, the queue thread will unlock when frame is free */
    // spin_unlock(&frame->frame_lock);
    return 0;
}

#endif
