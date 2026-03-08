#include <errno.h>
#include <io.h>
#include <string.h>
#include <sys.h>
#include <types.h>

#include <kernel/subsys/net.h>

void print_network_device(const char *path, const char *name) {
    handle_t net;
    int res;
    struct net_device_info info;
    struct net_ifaddr ifaddr;

    if ((res = open(&net, path, READ)) != 0) {
        printf("%s: Unable to open: %s\n", name, errno(res));
        return;
    }

    if ((res = ioctl(net, NET_IOCTL_INFO, &info)) != 0) {
        printf("%s: Unable to ioctl: %s\n", name, errno(res));
        return;
    }

    printf("%s: %s: link %d, flags 0x%x, mac %x:%x:%x:%x:%x:%x\n", name, info.name, info.link_state, info.flags, info.mac_addr[0],
           info.mac_addr[1], info.mac_addr[2], info.mac_addr[3], info.mac_addr[4], info.mac_addr[5]);

    /* Fetch all ifaddrs */
    while (ioctl(net, NET_IOCTL_IFADDR_NEXT, &ifaddr) > 0) {
        if (ifaddr.family == NET_IF_INET) {
            printf("  inet: %d.%d.%d.%d\n", ifaddr.address.ipv4[0], ifaddr.address.ipv4[1], ifaddr.address.ipv4[2],
                   ifaddr.address.ipv4[3]);
        } else if (ifaddr.family == NET_IF_INET6) {
            printf("  inet6: ...\n");
        } else {
            printf("  invalid: ...\n");
        }
    }

    close(net);
}

int main(int argc, char *argv[]) {
    struct fs_node node = {0};
    int found_files = 0;
    char buf[128] = "/devices/network/";

    /* Iterate through all network devices and fetch info */
    while (readdir("/devices/network", &node) > 0) {
        memcpy(buf + 17, node.name, strlen(node.name));
        print_network_device(buf, node.name);
        found_files = 1;
    }

    if (!found_files)
        printf("%s: No network devices found.\n", get_name());

    return 0;
}
