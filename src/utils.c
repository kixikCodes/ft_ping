#include "ft_ping.h"

u_int16_t   checksum(void *b, int len) {
    u_int16_t  *buf = b;
    u_int32_t  sum = 0;
    u_int16_t  result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

char    *dns_lookup(char *host, address_t *address_cont) {
    struct hostent  *host_entity;
    char            *ip = (char *)malloc(NI_MAXHOST * sizeof(char));

    if ((host_entity = gethostbyname(host)) == NULL){
        printf("ft_ping: %s: Name or service not known\n", host);
        exit(EXIT_FAILURE);
    }
    ft_strcpy(ip, inet_ntoa(*(struct in_addr *)host_entity->h_addr));
    (*address_cont).sin_family = host_entity->h_addrtype;
    (*address_cont).sin_port = htons(PORT_NUMBER);
    (*address_cont).sin_addr.s_addr = *(long *)host_entity->h_addr;
    return ip;
}

char    *rev_dns_lookup(char *ip_addr) {
    address_t  temp_addr;
    socklen_t  len;
    char       buf[NI_MAXHOST], *ret_buf;

    temp_addr.sin_family = AF_INET;
    temp_addr.sin_addr.s_addr = inet_addr(ip_addr);
    len = sizeof(struct sockaddr_in);
    if (getnameinfo((struct sockaddr *)&temp_addr, len, buf, sizeof(buf), NULL, 0, NI_NAMEREQD)) {
        printf("ft_ping: error: could not resolve reverse lookup of hostname\n");
        return NULL;
    }
    ret_buf = (char *)malloc((ft_strlen(buf) + 1) * sizeof(char));
    ft_strcpy(ret_buf, buf);
    return ret_buf;
}
