#include <net.h>
#include <sys.h>

#define NET_SYS_CREATE   0x0
#define NET_SYS_FREE     0x1
#define NET_SYS_SENDTO   0x2
#define NET_SYS_RECVFROM 0x3
#define NET_SYS_BIND     0x4
#define NET_SYS_CONNECT  0x5

struct net_sys_command {
    uint8_t id;
    union {
        struct {
            uint8_t ipn;
            uint16_t ethtype;
        } create;

        struct {
            struct net_sock_addr addr;
        } transport;

        /* Data flow (recv/send) */
        struct {
            void *buffer;
            size_t buffer_sz;
            struct net_sock_addr addr;

            /* For recv */
            uint32_t timeout_us;
            uint16_t flags;
        } flow;
    } as;
} __attribute__((packed));

int sockbind(handle_t sock_handle, struct net_sock_addr addr) {
    struct net_sys_command cmd = {.id = NET_SYS_BIND, .as.transport = {.addr = addr}};
    return net_sys(&cmd, &sock_handle);
}

int sockconnect(handle_t sock_handle, struct net_sock_addr addr) {
    struct net_sys_command cmd = {.id = NET_SYS_CONNECT, .as.transport = {.addr = addr}};
    return net_sys(&cmd, &sock_handle);
}

int sockcreate(handle_t *sock_handle, uint8_t ip_number, uint16_t ethtype) {
    struct net_sys_command cmd = {.id = NET_SYS_CREATE, .as.create = {.ipn = ip_number, .ethtype = ethtype}};
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

int sockrecvfrom(handle_t sock_handle, void *result_buffer, size_t result_size, struct net_sock_addr addr,
                 uint32_t timeout_us, uint16_t flags) {
    struct net_sys_command cmd = {.id = NET_SYS_RECVFROM,
                                  .as.flow = {
                                      .buffer = result_buffer,
                                      .buffer_sz = result_size,
                                      .addr = addr,
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
