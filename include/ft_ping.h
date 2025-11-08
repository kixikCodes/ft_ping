#ifndef FT_PING_H
#define FT_PING_H

// Special Settings (for netdb header to allow older macros)
#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

// Libraries and Headers

#include "../libft/include/libft.h"
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

// Proprocessor Defines

#define PING_PACKET_SIZE 64
#define PING_INTERVAL_MCRS 1000000
#define PORT_NUMBER 0

// Type Definitions

typedef struct sockaddr_in  t_ipaddr;
typedef struct sockaddr     t_sockaddr;
typedef struct in_addr      t_intraddr;
typedef struct iphdr        t_ipheader;
typedef struct icmphdr      t_icmp;
typedef struct hostent      t_hostent;
typedef struct timeval      t_time;
typedef enum e_errors {
    ERR_SOCKET,
    ERR_DNS,
    ERR_REVDNS,
    ERR_SEND,
    ERR_RECV,
    ERR_TTL,
    ERR_ARGS,
    ERR_INVALID,
    ERR_MEMORY,
    ERR_UNKNOWN
}   t_error;

// Functions Prototypes

void        display_help(void);
u_int16_t   checksum(void *b, int len);
char        *dns_lookup(const char *host, t_ipaddr *address_cont);
char        *rev_dns_lookup(char *ip_addr);
void        error(t_error err_type, const char *context);
void        build_icmp_request(t_icmp *icmp_hdr, unsigned int seq_no, pid_t pid);
ssize_t     send_icmp_request(int sockfd, t_ipaddr *addr, char *packet);
ssize_t     receive_icmp_reply(int sockfd, t_ipaddr *r_addr, char *recv_buf);
void        print_icmp_v(t_icmp *icmp_resp, t_ipaddr *r_addr, ssize_t recvd, int ip_hdr_len);

#endif /* FT_PING_H */
