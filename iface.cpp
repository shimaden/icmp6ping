#include "iface.h"

#include <stdio.h>
#include <string.h>

#include <fstream>

/* The 4th field of /proc/net/if_inet6 */
#define IPV6_ADDR_GLOBAL        0x0000U
#define IPV6_ADDR_LOOPBACK      0x0010U
#define IPV6_ADDR_LINKLOCAL     0x0020U
#define IPV6_ADDR_SITELOCAL     0x0040U
#define IPV6_ADDR_COMPATv4      0x0080U



static int str_to_ip6_addr(struct in6_addr *ip6_addr, const char *ip6_addr_str)
{
    char str[sizeof(struct in6_addr) * 2 + sizeof(struct in6_addr) + 1];
    const int len = strlen(ip6_addr_str);
    int index = 0;
    uint8_t *p = ip6_addr->s6_addr;

    for(int i = 0 ; i < len - 1 ; i += 2)
    {
        str[index++] = *(ip6_addr_str + i);
        str[index++] = *(ip6_addr_str + i + 1);
        str[index++] = ' ';
    }
    str[index] = '\0';
    int num = sscanf(str, "%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx "
                      "%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx",
                &p[0], &p[1], &p[2],  &p[3],  &p[4],  &p[5],  &p[6],  &p[7],
                &p[8], &p[9], &p[10], &p[11], &p[12], &p[13], &p[14], &p[15]);

    return num == 16;
}

struct in6_addr *get_iface_ip6_addr(const char *ifname, struct in6_addr *ip6_addr)
{
    const char procfile[] = "/proc/net/if_inet6";
    struct in6_addr *result = ip6_addr;

    FILE *fp = fopen(procfile, "r");
    if(fp == NULL)
    {
        result = NULL;
    }
    else
    {
        char line[128 + IFNAMSIZ];

        while(fgets(line, sizeof line, fp) != NULL)
        {
            if(strstr(line, ifname) == NULL)
            {
                continue;
            }

            struct proc_net_if_inet6 if_inet6;
            char ip6_addr_str[(128 / 8) * 2];
            const int num = sscanf(line, "%s %x %x %x %x %s",
                            ip6_addr_str,
                            &if_inet6.index, &if_inet6.prefix_len,
                            &if_inet6.scope, &if_inet6.dad_status,
                            if_inet6.if_name);
            if(num != 6)
            {
                result = NULL;
                break;
            }

            const int success = str_to_ip6_addr(ip6_addr, ip6_addr_str);
            if(!success)
            {
                result = NULL;
                break;
            }

            if(if_inet6.scope == IPV6_ADDR_GLOBAL)
            {
                result = ip6_addr;
                break;
            }
        }
        fclose(fp);
    }

    return result;
}

bool enum_iface_addr_ip6(const char *device, std::vector<struct proc_net_if_inet6>& addr_list)
{
    const char procfile[] = "/proc/net/if_inet6";
    std::ifstream proc_file(procfile);
    std::string line;
    bool is_success = true;

    while(std::getline(proc_file, line))
    {
        struct proc_net_if_inet6 if_inet6;
        char ip6_addr_str[(128 / 8) * 2];
        const int num = sscanf(line.c_str(), "%s %x %x %x %x %s",
                        ip6_addr_str,
                        &if_inet6.index, &if_inet6.prefix_len,
                        &if_inet6.scope, &if_inet6.dad_status,
                        if_inet6.if_name);
        if(num != 6)
        {
            is_success = false;
            break;
        }
        struct in6_addr ip6_addr;
        const int success = str_to_ip6_addr(&ip6_addr, ip6_addr_str);
        if(!success)
        {
            is_success = false;
            break;
        }
        if_inet6.ip6_addr = ip6_addr;
        addr_list.push_back(if_inet6);
    }

    return is_success;
}
