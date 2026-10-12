#ifndef OMDROP_RADIO_RUNTIME_H
#define OMDROP_RADIO_RUNTIME_H

#include "radio_adapter.h"
#include "radio_session.h"

struct omdrop_radio_runtime {
  struct omdrop_radio_session *session;
  struct omdrop_radio_adapter adapter;
};

void omdrop_radio_runtime_init(
    struct omdrop_radio_runtime *runtime,
    struct omdrop_radio_session *session, struct awdl_state *awdl_state,
    omdrop_radio_ethernet_callback ethernet_callback,
    void *ethernet_context);

int omdrop_radio_runtime_start(struct omdrop_radio_runtime *runtime,
                               uint32_t host_features,
                               uint16_t max_frame_size);
int omdrop_radio_runtime_set_channel(
    struct omdrop_radio_runtime *runtime,
    const struct omdrop_radio_channel *channel);
int omdrop_radio_runtime_start_rx(struct omdrop_radio_runtime *runtime);
int omdrop_radio_runtime_stop_rx(struct omdrop_radio_runtime *runtime);
int omdrop_radio_runtime_pump(struct omdrop_radio_runtime *runtime);
int omdrop_radio_runtime_send_raw(struct omdrop_radio_runtime *runtime,
                                  const uint8_t *frame, size_t length,
                                  uint16_t channel, uint8_t rate);

#endif
