#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <arpa/inet.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netpacket/packet.h>

#include "main.h"
#include "iface.h"
#include "setup_socket.h"
#include "send_echo6.h"
#include "print_echo6.h"

#define IP6_VERSION 6
#define IP6_PACKET_SIZE (sizeof(struct ip6_hdr) + sizeof(struct icmp6_hdr) + 64)

pthread_mutex_t g_mutex;

struct sent_data
{
    int      sock_fd;
    uint8_t *packet;
};

static void usage(const char *cmd)
{
    printf("Usage: %s <iface> <IPv6-addr>\n", cmd);
}

static void compose_ip6_hdr(
    struct ip6_hdr *ip6_hdr,
    uint16_t payload_len,
    const struct in6_addr& ip6_src_addr,
    const struct in6_addr& ip6_dst_addr)
{
    /* IPv6 header */
    ip6_hdr->ip6_vfc  = IP6_VERSION * 0x10;
    ip6_hdr->ip6_plen = htons(payload_len);
    ip6_hdr->ip6_nxt  = IPPROTO_ICMPV6;
    ip6_hdr->ip6_hlim = 0xFF; /* Hop limit (TTL) */
    ip6_hdr->ip6_src  = ip6_src_addr;
    ip6_hdr->ip6_dst  = ip6_dst_addr;
}

static void compose_icmp6_hdr(
    struct icmp6_hdr *icmp6_hdr,
    uint8_t           icmp6_type,
    uint8_t           icmp6_code)
{
    /* ICMPv6 header */
    icmp6_hdr->icmp6_type  = icmp6_type;
    icmp6_hdr->icmp6_code  = icmp6_code;
    icmp6_hdr->icmp6_cksum = 0;
    /* icmp6_id: An optional identification field that can be *used to help 
     * in matching Echo Request and Echo Reply messages.
     */
    icmp6_hdr->icmp6_id  = 0;
    /* icmp6_seq: Additional optional data to be sent along with the message. 
     * If this is sent in the Echo Request it is copied into the Echo Reply 
     * to be sent back to the source.
     */
    icmp6_hdr->icmp6_seq = 0;
}


static bool is_ipv6(const struct ip6_hdr *ip6_hdr)
{
    return (ip6_hdr->ip6_vfc >> 4) == 6;
}

static bool is_icmp6(const struct ip6_hdr *ip6_hdr)
{
    return ip6_hdr->ip6_nxt == IPPROTO_ICMPV6;
}

static bool is_same_addr(const struct in6_addr& addr1, const struct in6_addr& addr2)
{
    return memcmp(addr1.s6_addr, addr2.s6_addr, sizeof(struct in6_addr)) == 0;
}

#if 0
static bool is_reply_to_me(const struct ip6_hdr *recv_ip6_hdr, const struct ip6_hdr *sent_ip6_hdr)
{
    bool yes = is_ipv6(recv_ip6_hdr);
    yes = yes && is_icmp6(recv_ip6_hdr);
    yes = yes && is_same_addr(recv_ip6_hdr->ip6_src, sent_ip6_hdr->ip6_dst);

    return yes;
}
#endif
static bool is_reply_to_me(const uint8_t *recv_packet, const uint8_t *sent_packet)
{
    const struct ip6_hdr *recv_ip6_hdr = (struct ip6_hdr *)recv_packet;
    const struct icmp6_hdr *recv_icmp6_hdr = (struct icmp6_hdr *)(recv_packet + sizeof(struct ip6_hdr));
    const struct ip6_hdr *sent_ip6_hdr = (struct ip6_hdr *)sent_packet;
    const struct icmp6_hdr *sent_icmp6_hdr = (struct icmp6_hdr *)(sent_packet + sizeof(struct ip6_hdr));

    bool yes = is_ipv6(recv_ip6_hdr);
    yes = yes && is_icmp6(recv_ip6_hdr);
    yes = yes && is_same_addr(recv_ip6_hdr->ip6_src, sent_ip6_hdr->ip6_dst);
    yes = yes && recv_icmp6_hdr->icmp6_id == sent_icmp6_hdr->icmp6_id;
    yes = yes && recv_icmp6_hdr->icmp6_seq == sent_icmp6_hdr->icmp6_seq;

    return yes;
}

/*
 * Receive only an ICMPv6 Echo reply packet.
 */
static int recv_echo_reply(const struct sent_data *sent_data)
{
    uint8_t recv_packet[8192];
    const struct ip6_hdr *recv_ip6_hdr = (struct ip6_hdr *)recv_packet;
    const struct ip6_hdr *sent_ip6_hdr = (struct ip6_hdr *)sent_data->packet;
    int size = 0;
    int flags = 0;

    size = recv(sent_data->sock_fd, recv_packet, sizeof recv_packet, flags);
    if(size == -1)
    {
        perror("recv");
        return -1;
    }
    //if(is_reply_to_me(recv_ip6_hdr, sent_ip6_hdr))
    if(is_reply_to_me(recv_packet, sent_data->packet))
    {
        print_echo_reply(recv_packet, size, sent_ip6_hdr->ip6_dst);
    }

    return size;
}

void *recv_thread_func(void *arg)
{
    struct sent_data sent_data;

    pthread_mutex_lock(&g_mutex);
    memcpy(&sent_data, arg, sizeof(struct sent_data));
    pthread_mutex_unlock(&g_mutex);

    while(true)
    {
        const int size = recv_echo_reply(&sent_data);
        if(size < 0)
        {
            pthread_mutex_lock(&g_mutex);
            fprintf(stderr, "recv_echo_reply(): error.\n");
            pthread_mutex_unlock(&g_mutex);
        }
    }
}

static uint16_t get_echo_id()
{
    uint16_t buf;
    std::ifstream f("/dev/random", std::ios::binary);
    f.read((char *)&buf, 2);
    return buf;
}

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* Command line arguments */
    char device[IFNAMSIZ];
    strncpy(device, argv[1], IFNAMSIZ - 1);
    device[IP6_PACKET_SIZE - 1] = '\0';

    const char *dst_addr_str = argv[2];

    struct sockaddr_ll sockaddr_ll;
    int sock_fd = setup_socket(device, sockaddr_ll);
    if(sock_fd < -1)
    {
        fprintf(stderr, "Error in setup_socket().\n");
        return EXIT_FAILURE;
    }

    /* Source address */
    struct in6_addr if_ip6_addr;
    struct in6_addr *ip6_addr_ptr = get_iface_ip6_addr(device, &if_ip6_addr);
    if(ip6_addr_ptr == NULL)
    {
        fprintf(stderr, "Global address not found on %s\n", device);
        return EXIT_FAILURE;
    }

    /* Destination address */
    struct in6_addr ip6_dst_addr;
    inet_pton(AF_INET6, dst_addr_str, &ip6_dst_addr);

    /* Init IPv6 packet structure. */
    uint8_t ip6_packet[IP6_PACKET_SIZE];
    memset(ip6_packet, 0, sizeof ip6_packet);

	/* Set data to send an echo request. */
    struct ip6_hdr *ip6_hdr = (struct ip6_hdr *)ip6_packet;
    struct icmp6_hdr *icmp6_hdr = (struct icmp6_hdr *)(ip6_packet + sizeof(struct ip6_hdr));

    /* IPv6 header */
    const uint16_t payload_len = sizeof(struct icmp6_hdr);
    compose_ip6_hdr(ip6_hdr, payload_len, if_ip6_addr, ip6_dst_addr);
    /* ICMPv6 header */
    compose_icmp6_hdr(icmp6_hdr, ICMP6_ECHO_REQUEST, 0);

    pthread_mutex_init(&g_mutex, NULL);

    /* Create a receive thread. */
    struct sent_data sent_data;
    sent_data.sock_fd = sock_fd;
    sent_data.packet  = ip6_packet;
    pthread_t thread_id;
    int thread_ret = pthread_create(&thread_id, NULL, recv_thread_func, (void*)&sent_data);
    if(thread_ret != 0)
    {
        perror("pthread_create()");
        return EXIT_FAILURE;
    }

    /* Send echo requests. */
    const uint16_t echo_id  = htons(get_echo_id());
    uint16_t echo_seq = 0;
    struct timespec req;
    struct timespec rem = req;
    req.tv_sec  = 1;
    req.tv_nsec = 0;

    char dst_addr_buf[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, ip6_hdr->ip6_src.s6_addr, dst_addr_buf, sizeof(dst_addr_buf));

    pthread_mutex_lock(&g_mutex);
    printf("PING %s(%s) %lu bytes\n", dst_addr_buf, dst_addr_buf, sizeof(struct ip6_hdr) + payload_len);
    pthread_mutex_unlock(&g_mutex);

    while(true)
    {
        pthread_mutex_lock(&g_mutex);
        const int echo_req_success = send_echo_req(
                                            sock_fd,
                                            sockaddr_ll,
                                            ip6_packet,
                                            payload_len,
                                            echo_id,
                                            echo_seq);
        pthread_mutex_unlock(&g_mutex);
        ++echo_seq;
        if(!echo_req_success)
        {
            pthread_mutex_lock(&g_mutex);
            fprintf(stderr, "Echo failed.\n");
            pthread_mutex_unlock(&g_mutex);
        }

        nanosleep(&req, &rem); // Wait until the next ping.
    }

    return EXIT_SUCCESS;
}
