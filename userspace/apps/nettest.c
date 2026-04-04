#include <assert.h>
#include <errno.h>
#include <io.h>
#include <net.h>
#include <string.h>
#include <sys.h>

int main(int argc, char *argv[]) {
    int ret;
    pid_t pid;
    char *args[] = {"net0", "attach", "192.168.1.155", "255.255.255.0", "192.168.1.1", NULL};
    if ((ret = execpv(&pid, "/programs/ip.exe", 5, args)) != 0) {
        printf("ip: failed: %s\n", errno(ret));
    }
    waitpid(pid);

    /* Test UDP */
    struct net_sock_addr addr = {.addr_ln = 4, .address = {192, 168, 1, 132}, .identifier = 5005};
    struct net_sock_addr from;
    handle_t sock_handle;

    char buf[14];

    // assert(sockcreate(&sock_handle, NET_IPN_UDP, NET_ETHTYPE_IPV4, NULL) == 0, "sockcreate failed!");
    // assert(sockbind(sock_handle, addr) == 0, "sockbind failed!");

    // int r = sockrecvfrom(sock_handle, buf, 14, addr, 0, 0);
    // assert(r >= 0, "sockrecvfrom failed!");
    // printf("received %d bytes! %14s\n", r, buf);
    // assert(sockclose(sock_handle) == 0, "sockclose failed!");

    if (0) {
        const char *hello = "hello world!";
        assert(sockcreate(&sock_handle, NET_IPN_UDP, NET_ETHTYPE_IPV4, NULL) == 0, "sockcreate failed!");
        assert(sockconnect(sock_handle, addr) == 0, "sockconnect failed!");

        int r = socksendto(sock_handle, (void *)hello, strlen(hello), addr);
        assert(r == 0, "socksendto failed: %d", r);

        printf("sent!\n");
        r = sockrecvfrom(sock_handle, buf, 14, &from, 0, 0);
        assert(r >= 0, "sockrecvfrom failed!");
        printf("received %d bytes! %14s\n", r, buf);
    }

    assert(sockclose(sock_handle) == 0, "sockclose failed!");
    return 0;
}
