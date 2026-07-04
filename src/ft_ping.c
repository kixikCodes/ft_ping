// Main project file

#include "ft_ping.h"

bool    g_pingloop = true;
bool    g_opt_verbose = false;
bool    g_opt_flood = false;
bool    g_opt_quiet = false;
int     g_ttl_val = 64;

static char *parse_opts(int count, char **input) {
    char    *target = NULL;

    for (int i = 1; i < count; i++) {
        if (input[i][0] == '-') {
            if (ft_strlen(input[i]) != 2) // Enforce single character flags
                error(ERR_INVALID, input[i] + 1);
            switch (input[i][1]) {
                case 'v':
                    g_opt_verbose = true;
                    break;
                case 'f':
                    g_opt_flood = true;
                    break;
                case 'q': // Note: will negate ping logs regardless of other options
                    g_opt_quiet = true;
                    break;
                case 't': // Check and set custom TTL value
                    if (!input[i + 1] || !isnum(input[i + 1]))
                        error(ERR_INVALID, input[i] + 1);
                    int ttl = ft_atoi(input[i + 1]);
                    if (ttl < 1 || ttl > 255)
                        error(ERR_INVALID, input[i] + 1);
                    g_ttl_val = ttl;
                    i++;
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
    g_pingloop = false;
}

static void recv_and_log_res(int sockfd, pid_t pid, int *recv_count) {
    char        recv_buf[1024];
    char        addr_str[INET_ADDRSTRLEN];
    t_ipaddr    r_addr;
    ssize_t     recvd;

    while (g_pingloop) {
        recvd = receive_icmp_reply(sockfd, &r_addr, recv_buf, g_opt_flood);
        if (recvd <= 0 || errno == EINTR)
            return;
        inet_ntop(AF_INET, &r_addr.sin_addr, addr_str, sizeof(addr_str));

        t_ipheader  *ip_hdr = (t_ipheader *)recv_buf;
        int         ip_hdr_len = (ip_hdr->ihl & 0x0f) * 4;
        t_icmp      *icmp_resp = (t_icmp *)(recv_buf + ip_hdr_len);
        if (recvd - ip_hdr_len < (int)(sizeof(t_icmp) + sizeof(t_time)))
            continue;
        if (icmp_resp->type != ICMP_ECHOREPLY || ntohs(icmp_resp->un.echo.id) != (pid & 0xFFFF)) {
            if (g_opt_verbose && !g_opt_quiet)
                packet_warning(icmp_resp);
            continue;
        }

        u_int16_t received_cs = icmp_resp->checksum;
        icmp_resp->checksum = 0;
        if (received_cs != checksum(icmp_resp, recvd - ip_hdr_len)) {
            if (g_opt_verbose)
                fprintf(stderr, YELLOW "ft_ping: bad ICMP checksum: %s\n" RESET, addr_str);
            continue;
        }
        icmp_resp->checksum = received_cs;

        t_time tv_end;
        gettimeofday(&tv_end, NULL);
        t_time *t_sent = (t_time *)((char *)icmp_resp + sizeof(*icmp_resp));
        double rtt = (tv_end.tv_sec - t_sent->tv_sec) * 1000.0
                   + (tv_end.tv_usec - t_sent->tv_usec) / 1000.0;
        if (g_opt_flood && !g_opt_quiet) {
            printf("\b");
            fflush(stdout);
        } else if (g_opt_verbose && !g_opt_quiet) {
            log_verbose((long)(recvd - ip_hdr_len), addr_str,
                ntohs(icmp_resp->un.echo.sequence), ntohs(icmp_resp->un.echo.id),
                ip_hdr->ttl, rtt);
        } else if (!g_opt_quiet) {
            log_regular((long)(recvd - ip_hdr_len), addr_str,
                ntohs(icmp_resp->un.echo.sequence), ip_hdr->ttl, rtt);
        }
        (*recv_count)++;
        return;
    }
}

static void ping_loop(int sockfd, t_ipaddr *addr, char *ip, char *host) {
    char            send_buf[PING_PACKET_SIZE];
    unsigned int    seq_no = 0;
    int             msg_count = 0;
    int             recv_count = 0;
    pid_t           pid = getpid();

    setup_socket(sockfd, g_ttl_val);
    if (g_opt_verbose) {
        printf("ft_ping: sock4.fd: %d (socktype: %s), hints.ai_family: %s\n",
        sockfd, "SOCK_RAW", "AF_INET");
        printf("ft_ping: ai->ai_family: %s, ai->ai_canonname: '%s'\n",
            "AF_INET", host);
    }
    printf(BOLD "PING %s (%s) %d(%ld) bytes of data.\n" RESET,
        host, ip, PING_PACKET_SIZE, PING_PACKET_SIZE + sizeof(t_ipheader));

    while (g_pingloop) {
        build_icmp_request((t_icmp *)send_buf, seq_no++, pid);
        send_icmp_request(sockfd, addr, send_buf);
        if (g_opt_flood && !g_opt_quiet) {
            printf(".");
            fflush(stdout); // Justified by Bonus!
        }
        msg_count++;
        recv_and_log_res(sockfd, pid, &recv_count);
        if (!g_opt_flood)
            usleep(PING_INTERVAL_MCRS);
    }
    print_summary(host, msg_count, recv_count);
}

int main(int argc, char **argv) {
    char        *target;
    int         sockfd;
    char        *ip;
    t_ipaddr    address_cont;

    // Process input and resolve host
    if (argc < 2)
        error(ERR_ARGS, NULL);
    target = parse_opts(argc, argv);
    if (!target)
        error(ERR_ARGS, NULL);
    ip = dns_lookup(target, &address_cont);

    // Attempt pinging
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
        error(ERR_SOCKET, NULL);
    signal(SIGINT, interrupt_handler);
    ping_loop(sockfd, &address_cont, ip, target);

    // Cleanup
    gc_collect();
    return EXIT_SUCCESS;
}
