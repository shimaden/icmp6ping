#if !defined(PRINT_ECHO6__)
#define PRINT_ECHO6__

#include <netinet/ip6.h>
#include <netinet/icmp6.h>

extern void print_echo_reply(uint8_t *ip6_packet, int size, const struct in6_addr& dst_addr);

#endif
