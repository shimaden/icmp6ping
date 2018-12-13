#if !defined(CHECKSUM_H__2018)
#define CHECKSUM_H__2018

#include <nettle/nettle-types.h>

#if defined(__cplusplus)
extern "C" {
#endif

uint16_t icmp6checksum(uint8_t *packet, uint32_t size);

#if defined(__cplusplus)
};
#endif

#endif
