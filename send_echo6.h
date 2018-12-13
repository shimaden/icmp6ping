#if !defined(SEND_ECHO6_H__)
#define SEND_ECHO6_H__

#include <netpacket/packet.h>
#include <netinet/icmp6.h>
#include <netinet/ip6.h>

extern int send_echo_req(
            int sock_fd,
            struct sockaddr_ll& sockaddr_ll,
            uint8_t *ip6_packet,
            const uint16_t payload_len,
            uint16_t echo_id = 0,
            uint16_t echo_seq = 0);

#endif
