#ifndef OMDROP_VIRTUAL_ETHERNET_H
#define OMDROP_VIRTUAL_ETHERNET_H

#include <stddef.h>
#include <stdint.h>

#define OMDROP_VIRTUAL_ETHERNET_QUEUE_SIZE 16u
#define OMDROP_VIRTUAL_ETHERNET_MAX_FRAME 1518u

enum omdrop_virtual_ethernet_result {
  OMDROP_VIRTUAL_ETHERNET_OK = 1,
  OMDROP_VIRTUAL_ETHERNET_EMPTY = 0,
  OMDROP_VIRTUAL_ETHERNET_ERROR = -1
};

struct omdrop_virtual_ethernet_packet {
  uint8_t data[OMDROP_VIRTUAL_ETHERNET_MAX_FRAME];
  size_t length;
};

struct omdrop_virtual_ethernet_memory {
  struct omdrop_virtual_ethernet_packet rx[
      OMDROP_VIRTUAL_ETHERNET_QUEUE_SIZE];
  struct omdrop_virtual_ethernet_packet tx[
      OMDROP_VIRTUAL_ETHERNET_QUEUE_SIZE];
  size_t rx_head;
  size_t rx_count;
  size_t tx_head;
  size_t tx_count;
};

void omdrop_virtual_ethernet_memory_init(
    struct omdrop_virtual_ethernet_memory *device);

int omdrop_virtual_ethernet_host_send(
    struct omdrop_virtual_ethernet_memory *device, const uint8_t *data,
    size_t length);
int omdrop_virtual_ethernet_host_receive(
    struct omdrop_virtual_ethernet_memory *device, uint8_t *data,
    size_t capacity, size_t *length);

int omdrop_virtual_ethernet_read(
    void *context, uint8_t *data, size_t capacity, size_t *length);
int omdrop_virtual_ethernet_write(void *context, const uint8_t *data,
                                  size_t length);

#endif
