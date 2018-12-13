#include "send_echo6.h"

#include <stdio.h>

#include "checksum.h"

static int send_packet(int sock_fd, uint8_t *ip6_packet, int ip6_packet_len, const struct sockaddr_ll *sockaddr_ll)
{
    const int flags = 0;
    const int bytes = sendto(sock_fd,
                ip6_packet, ip6_packet_len, flags,
                (struct sockaddr *)sockaddr_ll, sizeof *sockaddr_ll);
    return bytes;
}

int send_echo_req(
            int sock_fd,
            struct sockaddr_ll& sockaddr_ll,
            uint8_t *ip6_packet,
            const uint16_t payload_len,
            uint16_t echo_id,
            uint16_t echo_seq)
{
    int ret = 1;
    //struct ip6_hdr *ip6_hdr = (struct ip6_hdr *)ip6_packet;
    struct icmp6_hdr *icmp6_hdr = (struct icmp6_hdr *)(ip6_packet + sizeof(struct ip6_hdr));

    /* ID and sequence of echo request. */
    icmp6_hdr->icmp6_id  = htons(echo_id);
    icmp6_hdr->icmp6_seq = htons(echo_seq);

    /* Checksum */
    const uint16_t cksum = icmp6checksum(ip6_packet, sizeof(struct ip6_hdr) + payload_len);
    icmp6_hdr->icmp6_cksum = htons(cksum);

    /* Send an ICMPv6 packet. */
    const int send_result = send_packet(sock_fd,
                ip6_packet, sizeof(struct ip6_hdr) + payload_len, &sockaddr_ll);
    if(send_result == -1)
    {
        perror("packet_send()");
        ret = 0;
    }

    return ret;
}
