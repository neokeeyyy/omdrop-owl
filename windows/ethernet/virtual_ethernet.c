#include "virtual_ethernet.h"

#include <string.h>

static size_t tail(size_t head, size_t count) {
  return (head + count) % OMDROP_VIRTUAL_ETHERNET_QUEUE_SIZE;
}

void omdrop_virtual_ethernet_memory_init(
    struct omdrop_virtual_ethernet_memory *device) {
  if (device == NULL) return;
  memset(device, 0, sizeof(*device));
}

int omdrop_virtual_ethernet_host_send(
    struct omdrop_virtual_ethernet_memory *device, const uint8_t *data,
    size_t length) {
  struct omdrop_virtual_ethernet_packet *packet;
  if (device == NULL || data == NULL || length == 0 ||
      length > OMDROP_VIRTUAL_ETHERNET_MAX_FRAME ||
      device->rx_count == OMDROP_VIRTUAL_ETHERNET_QUEUE_SIZE)
    return OMDROP_VIRTUAL_ETHERNET_ERROR;
  packet = &device->rx[tail(device->rx_head, device->rx_count++)];
  memcpy(packet->data, data, length);
  packet->length = length;
  return OMDROP_VIRTUAL_ETHERNET_OK;
}

int omdrop_virtual_ethernet_host_receive(
    struct omdrop_virtual_ethernet_memory *device, uint8_t *data,
    size_t capacity, size_t *length) {
  struct omdrop_virtual_ethernet_packet *packet;
  if (device == NULL || data == NULL || length == NULL || capacity == 0)
    return OMDROP_VIRTUAL_ETHERNET_ERROR;
  if (device->tx_count == 0) {
    *length = 0;
    return OMDROP_VIRTUAL_ETHERNET_EMPTY;
  }
  packet = &device->tx[device->tx_head];
  if (capacity < packet->length) return OMDROP_VIRTUAL_ETHERNET_ERROR;
  memcpy(data, packet->data, packet->length);
  *length = packet->length;
  device->tx_head = (device->tx_head + 1) %
                    OMDROP_VIRTUAL_ETHERNET_QUEUE_SIZE;
  device->tx_count--;
  return OMDROP_VIRTUAL_ETHERNET_OK;
}

int omdrop_virtual_ethernet_read(
    void *context, uint8_t *data, size_t capacity, size_t *length) {
  struct omdrop_virtual_ethernet_memory *device = context;
  struct omdrop_virtual_ethernet_packet *packet;
  if (device == NULL || data == NULL || length == NULL || capacity == 0)
    return OMDROP_VIRTUAL_ETHERNET_ERROR;
  if (device->rx_count == 0) {
    *length = 0;
    return OMDROP_VIRTUAL_ETHERNET_EMPTY;
  }
  packet = &device->rx[device->rx_head];
  if (capacity < packet->length) return OMDROP_VIRTUAL_ETHERNET_ERROR;
  memcpy(data, packet->data, packet->length);
  *length = packet->length;
  device->rx_head = (device->rx_head + 1) %
                    OMDROP_VIRTUAL_ETHERNET_QUEUE_SIZE;
  device->rx_count--;
  return OMDROP_VIRTUAL_ETHERNET_OK;
}

int omdrop_virtual_ethernet_write(void *context, const uint8_t *data,
                                  size_t length) {
  struct omdrop_virtual_ethernet_memory *device = context;
  struct omdrop_virtual_ethernet_packet *packet;
  if (device == NULL || data == NULL || length == 0 ||
      length > OMDROP_VIRTUAL_ETHERNET_MAX_FRAME ||
      device->tx_count == OMDROP_VIRTUAL_ETHERNET_QUEUE_SIZE)
    return OMDROP_VIRTUAL_ETHERNET_ERROR;
  packet = &device->tx[tail(device->tx_head, device->tx_count++)];
  memcpy(packet->data, data, length);
  packet->length = length;
  return OMDROP_VIRTUAL_ETHERNET_OK;
}
