#include <errno.h>
#include <io.h>
#include <lib.h>
#include <net.h>
#include <string.h>
#include <sys.h>

struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t seq;
    uint8_t data[40];
} __attribute__((packed));

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

int main(int argc, char *argv[]) {
    struct icmp_header icmp_header;
    time_t current_time, start_time;
    size_t total_packets = 0, success = 0;
    time_t icmp_seq_time[256] = {0};
    uint32_t current_seq = 0;
    uint16_t checksum;
    uint8_t ipv4[4];
    uint8_t icmp_data[40] = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!'};
    size_t recv_off, sent_size;
    handle_t sock_handle;
    int ret = 0;

    uptime(&start_time);

    if (argc < 2) {
        printf("usage: %s ip\n", argv[0]);
        return -1;
    }

    if (parse_ipv4(ipv4, argv[1]) != 0) {
        printf("%s: Invalid IP: %s\n", argv[0], argv[1]);
        return -1;
    }

    if ((ret = sockcreate(&sock_handle, NET_IPN_ICMP, NET_ETHTYPE_IPV4)) != 0) {
        printf("%s: Unable to create socket: %s\n", argv[0], errno(ret));
        return ret;
    }

    sent_size = sizeof(icmp_header);
    printf("PING %s: %d bytes of data.\n", argv[1], sent_size);

    /* Send pings */
    for (int i = 0; i < 4; i++) {
        uptime(&icmp_seq_time[current_seq % 256]);
        memset(&icmp_header, 0, sizeof(icmp_header));
        memcpy(&icmp_header.data[0], icmp_data, sizeof(icmp_data));
        icmp_header.type = 8;
        icmp_header.code = 0;
        icmp_header.seq = bigend16(current_seq);
        icmp_header.checksum = net_checksum(&icmp_header, sizeof(icmp_header));
        current_seq++;

        if ((ret = socksend(sock_handle, &icmp_header, sent_size, ipv4, sizeof(ipv4))) != 0) {
            printf("%s: Unable to send ping: %s\n", argv[0], errno(ret));
            return ret;
        }
        total_packets++;

        /* Wait for packets */
        recv_off = 0;
        while (recv_off < sent_size) {
            if ((ret = sockrecv(sock_handle, &icmp_header + recv_off, sent_size, ipv4, sizeof(ipv4), 0, 0)) < 0) {
                printf("%s: Unable to receive pong: %s\n", argv[0], errno(ret));
                return ret;
            }

            recv_off += ret;
            ret = 0;
        }
        uptime(&current_time);

        /* Validate packet */
        checksum = icmp_header.checksum;
        icmp_header.checksum = 0;
        if (icmp_header.type == 0 && icmp_header.code == 0 &&
            memcmp(&icmp_header.data[0], icmp_data, sizeof(icmp_data)) == 0 &&
            net_checksum(&icmp_header, sent_size) == checksum) {
            printf("%d bytes from %s: icmp_seq=%u time=%2f ms\n", recv_off, argv[1], icmp_header.seq,
                   ((double)current_time - (double)icmp_seq_time[icmp_header.seq % 256]) / 1000.0f);
            success++;
        } else {
            printf("%d bytes bad packet\n", recv_off);
        }

        /* One second delay */
        usleep(1000000);
    }

    uptime(&current_time);
    printf("%u packets transmitted, %d received, %d%% packet loss, time %2fms\n", total_packets, success,
           success > 0 ? 100 - 100 / (total_packets / success) : 100,
           ((double)current_time - (double)start_time) / 1000.0f);

    sockclose(sock_handle);
    return ret;
}
