#include "ft_ping.h"

int g_pingloop = 1;
int g_ping_interval = 1000000;

static void interrupt_handler(int interrupt) {
    (void)interrupt;
    g_pingloop = 0;
}

void    send_ping(int sockfd, address_t *addr, char *ip, char *host) {
    char send_buf[PING_PACKET_SIZE];
    char recv_buf[1024];
    address_t r_addr;
    socklen_t addr_len = sizeof(r_addr);
    int msg_count = 0;
    int msg_received = 0;
    struct timeval tv_start, tv_end;
    unsigned int seq_no = 0;
    pid_t pid = getpid();

    // Set TTL
    int ttl_val = 64;
    if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0) {
        printf("ft_ping: error: failed to configure socket TTL\n");
    }

    printf("PING %s (%s) %d(%ld) bytes of data.\n",
           host, ip, PING_PACKET_SIZE, PING_PACKET_SIZE + sizeof(struct iphdr));

    while (g_pingloop) {
        // build ICMP echo request
        struct icmphdr *icmp_hdr = (struct icmphdr *) send_buf;
        ft_memset(send_buf, 0, sizeof(send_buf));

        icmp_hdr->type = ICMP_ECHO;
        icmp_hdr->code = 0;
        icmp_hdr->un.echo.id = htons(pid & 0xFFFF);
        icmp_hdr->un.echo.sequence = htons(seq_no++);
        // fill data part with timestamp
        gettimeofday(&tv_start, NULL);
        ft_memcpy(send_buf + sizeof(struct icmphdr), &tv_start, sizeof(tv_start));
        // rest of data could be arbitrary / pad
        icmp_hdr->checksum = 0;
        icmp_hdr->checksum = checksum(icmp_hdr, PING_PACKET_SIZE);

        // send
        ssize_t sent = sendto(sockfd, send_buf, PING_PACKET_SIZE, 0,
                              (struct sockaddr *)addr, sizeof(*addr));
        if (sent <= 0)
            printf("ft_ping: error: packet failed to send\n");
        msg_count++;

        // receive
        ssize_t recvd = recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0,
                                 (struct sockaddr *)&r_addr, &addr_len);
        if (recvd <= 0) {
            if (errno == EINTR)
                continue;
            printf("ft_ping: error: no response\n");
        } else {
            // parse IP + ICMP
            struct iphdr *ip_hdr = (struct iphdr *) recv_buf;
            int ip_hdr_len = ip_hdr->ihl * 4;
            struct icmphdr *icmp_resp = (struct icmphdr *)(recv_buf + ip_hdr_len);

            if (icmp_resp->type == ICMP_ECHOREPLY &&
                ntohs(icmp_resp->un.echo.id) == (pid & 0xFFFF)) {
                gettimeofday(&tv_end, NULL);

                // compute RTT
                struct timeval *t_sent = (struct timeval *)(recv_buf + ip_hdr_len + sizeof(struct icmphdr));
                double rtt = (tv_end.tv_sec - t_sent->tv_sec) * 1000.0
                             + (tv_end.tv_usec - t_sent->tv_usec) / 1000.0;

                printf("%ld bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                       recvd - ip_hdr_len,
                       inet_ntoa(r_addr.sin_addr),
                       ntohs(icmp_resp->un.echo.sequence),
                       ip_hdr->ttl,
                       rtt);
                msg_received++;
            } else {
                // TODO: -v flag for verbose info
            }
        }
        usleep(g_ping_interval);
    }

    // Summary
    printf("\n--- %s ping statistics ---\n", host);
    printf("%d packets transmitted, %d received, %.1f%% packet loss\n",
           msg_count, msg_received,
           ((msg_count - msg_received) / (double)msg_count) * 100.0);
}

int main(int argc, char **argv) {
    int         sockfd;
    char        *ip_addr, *rev_hostname;
    address_t   address_cont;

    if (argc != 2) {
        printf("ft_ping: usage error: Destination address required\n");
        return EXIT_FAILURE;
    }
    ip_addr = dns_lookup(argv[1], &address_cont);
    if (ip_addr == NULL) 
    rev_hostname = rev_dns_lookup(ip_addr);
    printf("Trying to connect to '%s' IP: %s\n", argv[1], ip_addr);
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        printf("ft_ping: error: socket file descriptor not received\n");
        return EXIT_FAILURE;
    }
    signal(SIGINT, interrupt_handler);
    send_ping(sockfd, &address_cont, ip_addr, argv[1]);
    if (rev_hostname)
        free(rev_hostname); // FIXME: do something with this lol
    return EXIT_SUCCESS;
}
