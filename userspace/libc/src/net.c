#include <net.h>
#include <sys.h>

#define NET_SYS_CREATE 0x0
#define NET_SYS_FREE   0x1
#define NET_SYS_TX     0x2
#define NET_SYS_RX     0x3

struct net_sys_command {
    uint8_t id;
    union {
        struct {
            uint8_t ipn;
            uint16_t ethtype;
        } create;

        /* Data flow (recv/send) */
        struct {
            void *buffer;
            size_t buffer_sz;
            uint8_t *addr;
            size_t addr_sz;
        } flow;
    } as;
} __attribute__((packed));

int sockcreate(handle_t *sock_handle, uint8_t ip_number, uint16_t ethtype) {
    struct net_sys_command cmd = {.id = NET_SYS_CREATE, .as.create = {.ipn = ip_number, .ethtype = ethtype}};
    return net_sys(&cmd, sock_handle);
}

int socksend(handle_t sock_handle, void *data, size_t data_size, uint8_t *addr, size_t addr_size) {
    struct net_sys_command cmd = {.id = NET_SYS_TX,
                                  .as.flow = {
                                      .buffer = data,
                                      .buffer_sz = data_size,
                                      .addr = addr,
                                      .addr_sz = addr_size,
                                  }};
    return net_sys(&cmd, &sock_handle);
}

int sockrecv(handle_t sock_handle, void *result_buffer, size_t result_size, uint8_t *addr, size_t addr_size) {
    struct net_sys_command cmd = {.id = NET_SYS_RX,
                                  .as.flow = {
                                      .buffer = result_buffer,
                                      .buffer_sz = result_size,
                                      .addr = addr,
                                      .addr_sz = addr_size,
                                  }};
    return net_sys(&cmd, &sock_handle);
}

int sockclose(handle_t sock_handle) {
    struct net_sys_command cmd = {
        .id = NET_SYS_FREE,
    };
    return net_sys(&cmd, &sock_handle);
}
