#ifndef OMDROP_RADIO_ADAPTER_H
#define OMDROP_RADIO_ADAPTER_H

#include "radio_link.h"

#include "rx.h"

typedef int (*omdrop_radio_ethernet_callback)(void *context,
                                              const uint8_t *data,
                                              size_t length);

struct omdrop_radio_adapter {
  struct omdrop_radio_link *link;
  struct awdl_state *awdl_state;
  omdrop_radio_ethernet_callback ethernet_callback;
  void *ethernet_context;
};

void omdrop_radio_adapter_init(
    struct omdrop_radio_adapter *adapter, struct omdrop_radio_link *link,
    struct awdl_state *awdl_state,
    omdrop_radio_ethernet_callback ethernet_callback,
    void *ethernet_context);

int omdrop_radio_adapter_send_raw(struct omdrop_radio_adapter *adapter,
                                  const uint8_t *frame, size_t length,
                                  uint16_t channel, uint8_t rate);

int omdrop_radio_adapter_receive(struct omdrop_radio_adapter *adapter,
                                 const struct omdrop_radio_message *message);

#endif
