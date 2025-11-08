// Miscellanious project utils

#include "ft_ping.h"

void    display_help(void) {
    printf(
"Usage\n\
ping [options] <destination>\n\
\n\
Options:\n\
<destination>      DNS name or IP address\n\
-D                 print timestamps\n\
-f                 flood ping\n\
-?                 print help and exit\n\
-H                 force reverse DNS name resolution (useful for numeric\n\
                    destinations or for -f), override -n\n\
-n                 no reverse DNS name resolution, override -H\n\
-q                 quiet output\n\
-U                 print user-to-user latency\n\
-v                 verbose output\n"
    );
}

u_int16_t   checksum(void *b, int len) {
    u_int16_t  *buf = b;
    u_int32_t  sum = 0;
    u_int16_t  result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    result = ~sum;

    return result;
}

char    *dns_lookup(const char *host, t_ipaddr *address_cont) {
    t_hostent   *host_entity;
    t_intraddr  *addr;
    char        *ip;

    ip = (char *)malloc(INET_ADDRSTRLEN);
    if (!ip)
        error(ERR_MEMORY, NULL);
    host_entity = gethostbyname(host);
    if (!host_entity)
        error(ERR_DNS, host);
    addr = (t_intraddr *)host_entity->h_addr;
    if (inet_ntop(AF_INET, addr, ip, INET_ADDRSTRLEN) == NULL)
        error(ERR_DNS, host);
    address_cont->sin_family = host_entity->h_addrtype;
    address_cont->sin_port = htons(PORT_NUMBER);
    ft_memcpy(&address_cont->sin_addr, addr, sizeof(struct in_addr));

    return ip;
}

char    *rev_dns_lookup(char *ip_addr) {
    t_ipaddr        temp_addr;
    unsigned int    len;
    char            buf[NI_MAXHOST], *ret_buf;

    if (inet_pton(AF_INET, ip_addr, &temp_addr.sin_addr) <= 0)
        error(ERR_REVDNS, ip_addr);
    len = sizeof(t_ipaddr);
    if (getnameinfo((t_sockaddr *)&temp_addr, len, buf, sizeof(buf), NULL, 0, NI_NAMEREQD))
        error(ERR_REVDNS, ip_addr);
    ret_buf = (char *)malloc((ft_strlen(buf) + 1) * sizeof(char));
    if (!ret_buf)
        error(ERR_MEMORY, NULL);
    ft_strcpy(ret_buf, buf);

    return ret_buf;
}
