#include <io.h>
#include <net.h>
#include <sys.h>
#include <string.h>
#include <errno.h>

struct dhcp_discover {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t client_addr;
    uint32_t your_addr;
    uint32_t server_addr;
    uint32_t gateway_addr;
    uint32_t client_hardware_addr[4];
    uint8_t zeros[192];
    uint32_t magic_cookie;
} __attribute__((packed));

int main(int argc, char const *argv[]) {
    int ret = 0;
    handle_t sock_server = 0, sock_client = 0;
    pid_t pid;
    struct net_device_info info;
    struct dhcp_discover discover;
    struct net_sock_addr dhcp_server = {.addr_ln = 4, .address = {255, 255, 255, 255}, .identifier = 67};
    struct net_sock_addr dhcp_client = {.addr_ln = 4, .address = {0, 0, 0, 0}, .identifier = 68};
    if (argc < 2) {
        printf("usage: %s device\n", argv[0]);
        return -1;
    }

    /* Firstly fetch NIC MAC */
    if ((ret = net_info(argv[1], &info)) != 0) {
        printf("%s: Unable fetch %s info: %s\n", argv[0], argv[1], errno(ret));
        goto end;
    }

    /* Setup network device */
    char *ip_args[] = {(char*)argv[1], "attach", "0.0.0.0", "0.0.0.0", "255.255.255.255", NULL};
    if ((ret = execpv(&pid, "/programs/ip.exe", 5, ip_args)) != 0) {
        printf("%s: Failed to call IP.exe: %s\n", errno(ret));
    }
    waitpid(pid);

    /* Create the server connection and client bind sockets */
    if ((ret = sockcreate(&sock_server, NET_IPN_UDP, NET_ETHTYPE_IPV4, argv[1])) != 0 ||
        (ret = sockcreate(&sock_client, NET_IPN_UDP, NET_ETHTYPE_IPV4, argv[1])) != 0) {
        printf("%s: Unable to create sockets for %s: %s\n", argv[0], argv[1], errno(ret));
        goto end;
    }

    /* Connect and bind sockets */
    if ((ret = sockconnect(sock_server, dhcp_server)) != 0 || (ret = sockbind(sock_server, dhcp_client)) != 0) {
        printf("%s: Unable to connect/bind sockets: %s\n", argv[0], errno(ret));
        goto end;
    }

    /* Now transmit discover packet */
    discover.op = 0x1;
    discover.htype = 0x01;
    discover.hlen = 0x06;
    discover.hops = 0x00;
    discover.xid = bigend32(0x3903F326);
    discover.secs = bigend16(0x0000);
    discover.flags = bigend16(0x0000);
    discover.client_addr = 0x00;
    discover.your_addr = 0x00;
    discover.server_addr = 0x00;
    discover.gateway_addr = 0x00;
    memcpy(&discover.client_hardware_addr[0], info.mac_addr, 6);
    memset(&discover.zeros[0], 0, sizeof(discover.zeros));
    discover.magic_cookie = bigend32(0x63825363);
    if ((ret = socksendto(sock_server, &discover, sizeof(discover), dhcp_server)) != 0) {
        printf("%s: Unable to send DHCP discover: %s\n", argv[0], errno(ret));
        goto end;
    }

    /* Listen for offer */

    /* Send the request */

    /* Wait for ack */

    // assert( == 0, "sockcreate failed!");
    // assert(sockconnect(sock_handle, addr) == 0, "sockconnect failed!");

    // int r = socksendto(sock_handle, (void *)hello, strlen(hello), addr);
    // assert(r == 0, "socksendto failed: %d", r);

    // printf("sent!\n");
    // r = sockrecvfrom(sock_handle, buf, 14, &from, 0, 0);
    // assert(r >= 0, "sockrecvfrom failed!");
    // printf("received %d bytes! %14s\n", r, buf);

    /* Hooray! */

end:
    /* Cleanup 0.0.0.0 */
    char *cleanup_ip_args[] = {(char*)argv[1], "detach", "0.0.0.0", NULL};
    if ((ret = execpv(&pid, "/programs/ip.exe", 3, cleanup_ip_args)) != 0) {
        printf("%s: Failed to call IP.exe: %s\n", errno(ret));
    }
    waitpid(pid);

    if (sock_server)
        sockclose(sock_server);
    if (sock_client)
        sockclose(sock_client);
    return ret;
}
