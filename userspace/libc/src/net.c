#include <net.h>
#include <string.h>
#include <sys.h>

#define NET_SYS_CREATE       0x0
#define NET_SYS_FREE         0x1
#define NET_SYS_SENDTO       0x2
#define NET_SYS_RECVFROM     0x3
#define NET_SYS_BIND         0x4
#define NET_SYS_CONNECT      0x5
#define NET_SYS_REQ_NIC_INFO 0x6
#define NET_SYS_REQ_HOSTNAME 0x7
#define NET_SYS_SET_HOSTNAME 0x8

struct net_sys_command {
    uint8_t id;
    union {
        struct {
            char *result;
        } req_hostname;

        struct {
            char hostname[64];
        } set_hostname;

        struct {
            char net_device[48];
            struct net_device_info *result;
        } nic_info;

        struct {
            uint8_t ipn;
            uint16_t ethtype;
            char net_device[48];
        } create;

        struct {
            struct net_sock_addr addr;
        } transport;

        /* Data flow (recv/send) */
        struct {
            void *buffer;
            size_t buffer_sz;
            struct net_sock_addr addr;
            struct net_sock_addr *from;

            /* For recv */
            uint32_t timeout_us;
            uint16_t flags;
        } flow;
    } as;
} __attribute__((packed));

int set_hostname(char *hostname) {
    struct net_sys_command cmd = {.id = NET_SYS_SET_HOSTNAME};
    cmd.as.nic_info.net_device[0] = 0;
    if (hostname)
        memcpy(cmd.as.set_hostname.hostname, hostname,
               strlen(hostname) + 1 > sizeof(cmd.as.set_hostname.hostname) ? sizeof(cmd.as.set_hostname.hostname)
                                                                   : strlen(hostname) + 1);
    cmd.as.set_hostname.hostname[sizeof(cmd.as.set_hostname.hostname) - 1] = 0;
    return net_sys(&cmd, NULL);
}

int get_hostname(char *result) {
    struct net_sys_command cmd = {.id = NET_SYS_REQ_HOSTNAME, .as.req_hostname = {.result = result}};
    return net_sys(&cmd, NULL);
}

int net_info(const char *name, struct net_device_info *result) {
    struct net_sys_command cmd = {.id = NET_SYS_REQ_NIC_INFO, .as.nic_info = {.result = result}};
    cmd.as.nic_info.net_device[0] = 0;
    if (name)
        memcpy(cmd.as.nic_info.net_device, name,
               strlen(name) + 1 > sizeof(cmd.as.nic_info.net_device) ? sizeof(cmd.as.nic_info.net_device)
                                                                   : strlen(name) + 1);
    cmd.as.nic_info.net_device[sizeof(cmd.as.nic_info.net_device) - 1] = 0;
    return net_sys(&cmd, NULL);
}

int sockbind(handle_t sock_handle, struct net_sock_addr addr) {
    struct net_sys_command cmd = {.id = NET_SYS_BIND, .as.transport = {.addr = addr}};
    return net_sys(&cmd, &sock_handle);
}

int sockconnect(handle_t sock_handle, struct net_sock_addr addr) {
    struct net_sys_command cmd = {.id = NET_SYS_CONNECT, .as.transport = {.addr = addr}};
    return net_sys(&cmd, &sock_handle);
}

int sockcreate(handle_t *sock_handle, uint8_t ip_number, uint16_t ethtype, const char *net_device) {
    struct net_sys_command cmd = {.id = NET_SYS_CREATE, .as.create = {.ipn = ip_number, .ethtype = ethtype}};
    cmd.as.create.net_device[0] = 0;
    if (net_device)
        memcpy(cmd.as.create.net_device, net_device,
               strlen(net_device) + 1 > sizeof(cmd.as.create.net_device) ? sizeof(cmd.as.create.net_device)
                                                                         : strlen(net_device) + 1);
    cmd.as.create.net_device[sizeof(cmd.as.create.net_device) - 1] = 0;
    return net_sys(&cmd, sock_handle);
}

int socksendto(handle_t sock_handle, void *data, size_t data_size, struct net_sock_addr addr) {
    struct net_sys_command cmd = {.id = NET_SYS_SENDTO,
                                  .as.flow = {
                                      .buffer = data,
                                      .buffer_sz = data_size,
                                      .addr = addr,
                                  }};
    return net_sys(&cmd, &sock_handle);
}

int sockrecvfrom(handle_t sock_handle, void *result_buffer, size_t result_size, struct net_sock_addr *from,
                 uint32_t timeout_us, uint16_t flags) {
    struct net_sys_command cmd = {.id = NET_SYS_RECVFROM,
                                  .as.flow = {
                                      .buffer = result_buffer,
                                      .buffer_sz = result_size,
                                      .from = from,
                                      .timeout_us = timeout_us,
                                      .flags = flags,
                                  }};
    return net_sys(&cmd, &sock_handle);
}

int sockclose(handle_t sock_handle) {
    struct net_sys_command cmd = {
        .id = NET_SYS_FREE,
    };
    return net_sys(&cmd, &sock_handle);
}
