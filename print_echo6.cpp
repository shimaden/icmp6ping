#include "print_echo6.h"

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "main.h"

void print_echo_reply(uint8_t *ip6_packet, int size, const struct in6_addr& dst_addr)
{
    if(size < (int)(sizeof(struct ip6_hdr) + sizeof(struct icmp6_hdr)))
    {
        pthread_mutex_lock(&g_mutex);
        fprintf(stderr, "Packet too small.\n");
        pthread_mutex_unlock(&g_mutex);
        return;
    }
    const struct ip6_hdr *ip6_hdr = (struct ip6_hdr *)ip6_packet;
    const struct icmp6_hdr *icmp6_hdr = (struct icmp6_hdr *)(ip6_packet + sizeof(struct ip6_hdr));
    uint16_t payload_len = ntohs(ip6_hdr->ip6_plen);
    if((int)(payload_len + sizeof(struct ip6_hdr)) > size)
    {
        pthread_mutex_lock(&g_mutex);
        fprintf(stderr, "Received packet size mismatch.\n");
        pthread_mutex_unlock(&g_mutex);
        return;
    }

    char ip6_src_str[INET6_ADDRSTRLEN]; 
    inet_ntop(AF_INET6, ip6_hdr->ip6_src.s6_addr, ip6_src_str, sizeof ip6_src_str);
    pthread_mutex_lock(&g_mutex);
    printf("%ld bytes from %s: icmp_seq=%d ttl=%d icmp_id=0x%04X\n",
            sizeof(struct ip6_hdr) + payload_len, ip6_src_str,
            ntohs(icmp6_hdr->icmp6_seq),
            (int)ip6_hdr->ip6_hlim, /* 8-bit value */
            (int)ntohs(icmp6_hdr->icmp6_id)
    );
    pthread_mutex_unlock(&g_mutex);
}

