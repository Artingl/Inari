#include <sys.h>
#include <io.h>
#include <net.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t data;
} __attribute__((packed));

int main(int argc, char *argv[])
{
    int ret;
    pid_t pid;
    char *args[] = { "net0", "attach", "192.168.99.155", "255.255.255.0", "192.168.99.1", NULL };
    if ((ret = execpv(&pid, "/programs/ip.exe", 5, args)) != 0) {
        printf("ip: failed: %s\n", errno(ret));
    }
    waitpid(pid);

    /* Test icmp */
    uint8_t addr[] = { 192, 168, 99, 10 };
    handle_t sock_handle;

    struct icmp_header icmp_header;
    memset(&icmp_header, 0, sizeof(icmp_header));
    icmp_header.type = 8;
    icmp_header.code = 0;
    icmp_header.checksum = net_checksum(&icmp_header, sizeof(icmp_header));

    assert(sockcreate(&sock_handle, NET_IPN_ICMP, NET_ETHTYPE_IPV4) == 0, "sockcreate failed!");
    int r = socksend(sock_handle, &icmp_header, sizeof(icmp_header), addr, sizeof(addr));
    assert(r == 0, "socksend failed: %d", r);

    printf("sent!\n");
    assert(sockrecv(sock_handle, &icmp_header, sizeof(icmp_header), addr, sizeof(addr)) == 0, "sockrecv failed!");
    printf("received!\n");

    assert(sockclose(sock_handle) == 0, "sockclose failed!");
    return 0;
}
