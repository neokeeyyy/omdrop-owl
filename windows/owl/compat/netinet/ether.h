#ifndef OMDROP_WINDOWS_NETINET_ETHER_H
#define OMDROP_WINDOWS_NETINET_ETHER_H

#include <stdio.h>
#include <string.h>

#include "../net/ethernet.h"

static inline char *ether_ntoa(const struct ether_addr *address) {
  static char text[18];
  if (address == NULL) return NULL;
  snprintf(text, sizeof(text), "%02x:%02x:%02x:%02x:%02x:%02x",
           address->ether_addr_octet[0], address->ether_addr_octet[1],
           address->ether_addr_octet[2], address->ether_addr_octet[3],
           address->ether_addr_octet[4], address->ether_addr_octet[5]);
  return text;
}

#endif
