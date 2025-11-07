#ifndef FT_PING_H
#define FT_PING_H

#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE // This is required for some outdated shit.

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

#define PING_PACKET_SIZE 64
#define PORT_NUMBER 0

typedef struct sockaddr_in  address_t;

u_int16_t   checksum(void *b, int len);
char        *dns_lookup(char *host, address_t *address_cont);
char        *rev_dns_lookup(char *ip_addr);

#endif /* FT_PING_H */
