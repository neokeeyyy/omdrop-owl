#ifndef OMDROP_WINDOWS_NET_ETHERNET_H
#define OMDROP_WINDOWS_NET_ETHERNET_H

#include <stdint.h>

struct ether_addr {
  uint8_t ether_addr_octet[6];
};

#define ETHER_ADDR_LEN 6
#define ETHER_MAX_LEN 1518

#endif
