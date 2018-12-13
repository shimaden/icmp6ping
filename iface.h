#if !defined(IFACE_H__)
#define IFACE_H__

#include <net/if.h>
#include <netinet/ip6.h>
#include <vector>

struct proc_net_if_inet6
{
    struct in6_addr ip6_addr;
    /* The type of the 4 below members must be unsigned int for sscanf() */
    unsigned int index;
    unsigned int prefix_len;
    unsigned int scope;
    unsigned int dad_status;
    char if_name[IFNAMSIZ];
};

extern struct in6_addr *get_iface_ip6_addr(const char *ifname, struct in6_addr *ip6_addr);
extern bool enum_iface_addr_ip6(const char *device, std::vector<struct proc_net_if_inet6>& addr_list);

#endif
