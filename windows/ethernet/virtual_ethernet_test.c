#include "virtual_ethernet.h"

#include <assert.h>
#include <string.h>

int main(void) {
  struct omdrop_virtual_ethernet_memory device;
  uint8_t input[] = {0, 1, 2, 3};
  uint8_t output[sizeof(input)];
  size_t length;

  omdrop_virtual_ethernet_memory_init(&device);
  assert(omdrop_virtual_ethernet_host_send(&device, input, sizeof(input)) ==
         OMDROP_VIRTUAL_ETHERNET_OK);
  assert(omdrop_virtual_ethernet_read(&device, output, sizeof(output),
                                      &length) == OMDROP_VIRTUAL_ETHERNET_OK);
  assert(length == sizeof(input));
  assert(memcmp(input, output, length) == 0);
  assert(omdrop_virtual_ethernet_read(&device, output, sizeof(output),
                                      &length) == OMDROP_VIRTUAL_ETHERNET_EMPTY);

  assert(omdrop_virtual_ethernet_write(&device, input, sizeof(input)) ==
         OMDROP_VIRTUAL_ETHERNET_OK);
  memset(output, 0, sizeof(output));
  assert(omdrop_virtual_ethernet_host_receive(&device, output, sizeof(output),
                                              &length) ==
         OMDROP_VIRTUAL_ETHERNET_OK);
  assert(length == sizeof(input));
  assert(memcmp(input, output, length) == 0);
  assert(omdrop_virtual_ethernet_host_send(
             &device, input, OMDROP_VIRTUAL_ETHERNET_MAX_FRAME + 1) ==
         OMDROP_VIRTUAL_ETHERNET_ERROR);
  assert(omdrop_virtual_ethernet_host_receive(&device, output, 1, &length) ==
         OMDROP_VIRTUAL_ETHERNET_EMPTY);
  return 0;
}
