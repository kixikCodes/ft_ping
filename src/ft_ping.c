// Main project file

#include "ft_ping.h"

bool    g_pingloop = true;
bool    g_opt_verbose = false;

static const char   *parse_opts(int count, char **input) {
    const char  *target = NULL;

    for (int i = 1; i < count; i++) {
        if (input[i][0] == '-') {
            switch (input[i][1])
            {
            case 'v':
                g_opt_verbose = true;
                break;
            case '?':
                display_help();
                exit(EXIT_SUCCESS);
            default:
                error(ERR_INVALID, input[i] + 1);
            }
        } else
            target = input[i];
    }
    return target;
}

static void interrupt_handler(int interrupt) {
    (void)interrupt;
    g_pingloop = 0;
}

static void setup_socket(int sockfd) {
    int ttl_val = 64;
    if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0)
        error(ERR_SOCKET, NULL);
}

static void catch_response(int sockfd, pid_t pid, int *recv_count, bool verbose) {
    char        recv_buf[1024];
    char        addr_str[INET_ADDRSTRLEN];
    t_ipaddr    r_addr;
    ssize_t     recvd;

    recvd = receive_icmp_reply(sockfd, &r_addr, recv_buf);
    if (recvd <= 0) {
        if (errno == EINTR)
            return;
        error(ERR_RECV, NULL);
    }
    inet_ntop(AF_INET, &r_addr.sin_addr, addr_str, sizeof(addr_str));

    t_ipheader  *ip_hdr = (t_ipheader *)recv_buf;
    int         ip_hdr_len = (ip_hdr->ihl & 0x0f) * 4;
    t_icmp      *icmp_resp = (t_icmp *)(recv_buf + ip_hdr_len);

    if (recvd - ip_hdr_len >= (int)(sizeof(t_icmp) + sizeof(t_time))) {
        if (icmp_resp->type == ICMP_ECHOREPLY &&
            ntohs(icmp_resp->un.echo.id) == (pid & 0xFFFF)) {
            t_time  tv_end;
            gettimeofday(&tv_end, NULL);
            t_time  *t_sent = (t_time *)(recv_buf + ip_hdr_len + sizeof(t_icmp));
            double rtt = (tv_end.tv_sec - t_sent->tv_sec) * 1000.0
                       + (tv_end.tv_usec - t_sent->tv_usec) / 1000.0;
            if (verbose)
                print_icmp_v(icmp_resp, &r_addr, recvd, ip_hdr_len);
            else {
                printf("%ld bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                    (long)(recvd - ip_hdr_len),
                    addr_str,
                    ntohs(icmp_resp->un.echo.sequence),
                    ip_hdr->ttl,
                    rtt);
            }
            (*recv_count)++;
            return;
        }
    } else {
        if (verbose)
            fprintf(stderr, "ft_ping: received short ICMP packet from %s\n", addr_str);
        return;
    }
}

static void print_summary(const char *host, int msg_count, int msg_received) {
    printf("\n--- %s ping statistics ---\n", host);
    printf(BLUE "%d packets transmitted," GREEN " %d received," YELLOW " %.1f%% packet loss\n" RESET,
           msg_count, msg_received,
           ((msg_count - msg_received) / (double)msg_count) * 100.0);
}

static void ping_loop(int sockfd, t_ipaddr *addr, char *ip, const char *host) {
    char            send_buf[PING_PACKET_SIZE];
    unsigned int    seq_no = 0;
    int             msg_count = 0;
    int             recv_count = 0;
    pid_t           pid = getpid();

    setup_socket(sockfd);

    printf(BLUE "PING %s (%s) %d(%ld) bytes of data.\n" RESET,
           host, ip, PING_PACKET_SIZE, PING_PACKET_SIZE + sizeof(t_ipheader));

    while (g_pingloop) {
        build_icmp_request((t_icmp *)send_buf, seq_no++, pid);
        send_icmp_request(sockfd, addr, send_buf);
        msg_count++;
        catch_response(sockfd, pid, &recv_count, g_opt_verbose);
        usleep(PING_INTERVAL_MCRS);
    }

    print_summary(host, msg_count, recv_count);
}

int main(int argc, char **argv) {
    const char  *target;
    int         sockfd;
    char        *ip_addr, *rev_hostname;
    t_ipaddr    address_cont;

    // Process input and resolve host
    if (argc < 2)
        error(ERR_ARGS, NULL);
    target = parse_opts(argc, argv);
    if (!target)
        error(ERR_ARGS, NULL);
    ip_addr = dns_lookup(target, &address_cont);
    if (ip_addr == NULL) 
    rev_hostname = rev_dns_lookup(ip_addr);

    // Attempt pinging
    printf("Trying to connect to '%s' IP: %s\n", target, ip_addr);
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
        error(ERR_SOCKET, NULL);
    signal(SIGINT, interrupt_handler);
    ping_loop(sockfd, &address_cont, ip_addr, target);

    // Cleanup
    if (ip_addr)
        free(ip_addr);
    if (rev_hostname)
        free(rev_hostname);
    return EXIT_SUCCESS;
}
