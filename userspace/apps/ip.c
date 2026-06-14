#include <errno.h>
#include <io.h>
#include <lib.h>
#include <net.h>
#include <string.h>
#include <sys.h>
#include <types.h>

void print_network_device(const char *path) {
    handle_t net;
    int res;
    struct net_device_info info;
    struct net_ifaddr ifaddr;

    if ((res = open(&net, path, READ)) != 0) {
        printf("%s: Unable to open: %s\n", path, errno(res));
        return;
    }

    if ((res = ioctl(net, NET_IOCTL_INFO, &info)) != 0) {
        printf("%s: Unable to ioctl: %s\n", path, errno(res));
        return;
    }

    printf("%s: %s: link %d, flags 0x%x, mac %x:%x:%x:%x:%x:%x\n", info.name, info.hardware, info.link_state,
           info.flags, info.mac_addr[0], info.mac_addr[1], info.mac_addr[2], info.mac_addr[3], info.mac_addr[4],
           info.mac_addr[5]);

    /* Fetch all ifaddrs */
    while (ioctl(net, NET_IOCTL_IFADDR_NEXT, &ifaddr) > 0) {
        if (ifaddr.family == NET_IF_INET) {
            printf("  inet: %d.%d.%d.%d\tmask %d.%d.%d.%d\tgateway %d.%d.%d.%d\n", ifaddr.address.ipv4[0],
                   ifaddr.address.ipv4[1], ifaddr.address.ipv4[2], ifaddr.address.ipv4[3], ifaddr.netmask.ipv4[0],
                   ifaddr.netmask.ipv4[1], ifaddr.netmask.ipv4[2], ifaddr.netmask.ipv4[3], ifaddr.gateway.ipv4[0],
                   ifaddr.gateway.ipv4[1], ifaddr.gateway.ipv4[2], ifaddr.gateway.ipv4[3]);
        } else if (ifaddr.family == NET_IF_INET6) {
            printf("  inet6: ...\n");
        } else {
            printf("  invalid: ...\n");
        }
    }

    close(net);
}

int parse_ipv4(uint8_t *result, const char *ip) {
    /* Max ipv4 len is 15 chars */
    char buf[4] = {0};
    int off = 0, off1 = 0;
    for (size_t i = 0; i < 15 && ip[i]; i++) {
        if (off1 > 3)
            return -1;
        if (ip[i] == '.') {
            result[off++] = atoi(buf);
            buf[0] = 0;
            buf[1] = 0;
            buf[2] = 0;
            off1 = 0;
            continue;
        }
        buf[off1++] = ip[i];
    }
    result[off++] = atoi(buf);
    if (off == 4)
        return 0;

    return -1;
}

int detach(const char *device, const char *ip) {
    char buf[128] = "/devices/network/char_";
    memcpy(buf + 22, device, strlen(device) + 1);
    handle_t net;
    int res;
    struct net_ifaddr ifaddr = {
        .family = NET_IF_INET,
    };

    if (parse_ipv4(ifaddr.address.ipv4, ip) != 0) {
        printf("%s: Invalid address\n", device);
        return -1;
    }

    if ((res = open(&net, buf, READ)) != 0) {
        printf("%s: Unable to open: %s\n", device, errno(res));
        return -1;
    }

    if ((res = ioctl(net, NET_IOCTL_DETACH_IFADDR, &ifaddr)) != 0) {
        printf("%s: Unable to ioctl: %s\n", device, errno(res));
        return -1;
    }

    close(net);
    return 0;
}

int attach(const char *device, const char *ip, const char *mask, const char *gateway) {
    char buf[128] = "/devices/network/char_";
    memcpy(buf + 22, device, strlen(device) + 1);
    handle_t net;
    int res;
    struct net_ifaddr ifaddr = {
        .family = NET_IF_INET,
    };

    if (parse_ipv4(ifaddr.address.ipv4, ip) != 0) {
        printf("%s: Invalid IPv4 address\n", device);
        return -1;
    }

    if (parse_ipv4(ifaddr.netmask.ipv4, mask) != 0) {
        printf("%s: Invalid mask address\n", device);
        return -1;
    }

    if (parse_ipv4(ifaddr.gateway.ipv4, gateway) != 0) {
        printf("%s: Invalid gateway address\n", device);
        return -1;
    }

    if ((res = open(&net, buf, READ)) != 0) {
        printf("%s: Unable to open: %s\n", device, errno(res));
        return -1;
    }

    if ((res = ioctl(net, NET_IOCTL_ATTACH_IFADDR, &ifaddr)) != 0) {
        printf("%s: Unable to ioctl: %s\n", device, errno(res));
        return -1;
    }

    close(net);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc > 3) {
        const char *device = argv[1];
        if (strcmp(argv[2], "detach") == 0) {
            if (argc > 3) {
                return detach(device, argv[3]);
            } else {
                printf("usage: %s device detach ip\n", get_name());
                return -1;
            }
        } else if (strcmp(argv[2], "attach") == 0) {
            if (argc > 5) {
                return attach(device, argv[3], argv[4], argv[5]);
            } else {
                printf("usage: %s device attach ip mask gateway\n", get_name());
                return -1;
            }
        } else {
            printf("usage: %s [device] [attach/detach]", get_name());
            return -1;
        }
        return -1;
    }

    struct fs_node node = {0};
    int found_files = 0;
    char buf[128] = "/devices/network/";

    /* Iterate through all network devices and fetch info */
    while (readdir("/devices/network", &node) > 0) {
        memcpy(buf + 17, node.name, strlen(node.name) + 1);
        print_network_device(buf);
        found_files = 1;
    }

    if (!found_files)
        printf("%s: No network devices found.\n", get_name());

    return 0;
}
