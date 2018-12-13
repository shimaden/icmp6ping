#include "setup_socket.h"

#include <stdio.h>
#include <memory.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <netinet/if_ether.h>

static int bind_socket(int sock_fd, int if_index, struct sockaddr_ll& sockaddr_ll)
{
    memset(&sockaddr_ll, 0xff, sizeof(sockaddr_ll));
    sockaddr_ll.sll_family   = AF_PACKET;
    sockaddr_ll.sll_protocol = htons(ETH_P_IPV6);
    sockaddr_ll.sll_ifindex  = if_index;
    const int result = bind(sock_fd, (struct sockaddr *)&sockaddr_ll, sizeof(sockaddr_ll));
    return result;
}

static bool flush_recv_buf(int sock_fd)
{
	uint8_t buf[1024];
	int result;
    struct timeval t;

    int ret = 1;
    do
    {
	    fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock_fd, &readfds);

        memset(&t, 0, sizeof t);
        result = select(sock_fd + 1, &readfds, NULL, NULL, &t);
        if(result > 0)
        {
            recv(sock_fd, buf, sizeof buf, 0);
        }
        else if(result == -1)
        {
            perror("select()");
            ret = 0;
            break;
        }
    } while(result != 0);

    return ret;
}

int setup_socket(const char *device, struct sockaddr_ll& sockaddr_ll)
{
    /* Create a socket. */
    int sock_fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IPV6));
    if(sock_fd < 0)
    {
        perror("socket()");
        return -1;
    }

    /* Get the interface index for device. */
    struct ifreq if_request;
    memset(&if_request, 0, sizeof if_request);
    strncpy(if_request.ifr_name, device, IFNAMSIZ);
    if(ioctl(sock_fd, SIOCGIFINDEX, &if_request) == -1)
    {
        perror("ioctl SIOCGIFINDEX");
        close(sock_fd);
        return -1;
    }

    /* Bind the socket. */
    if(bind_socket(sock_fd, if_request.ifr_ifindex, sockaddr_ll) == -1)
    {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    /* I/F MAC address for sending */
    sockaddr_ll.sll_halen = IFHWADDRLEN;
    /* Set the ehternet broadcast address */
    memset(sockaddr_ll.sll_addr, 0xff, IFHWADDRLEN);

    /* Flush receive buffer. */
    const bool flush_success =flush_recv_buf(sock_fd);
    if(!flush_success)
    {
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}
