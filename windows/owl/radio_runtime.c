#include "radio_runtime.h"

#include <string.h>

static int runtime_message(void *context,
                           const struct omdrop_radio_message *message) {
  struct omdrop_radio_runtime *runtime = context;
  return omdrop_radio_adapter_receive(&runtime->adapter, message) >= RX_OK;
}

void omdrop_radio_runtime_init(
    struct omdrop_radio_runtime *runtime,
    struct omdrop_radio_session *session, struct awdl_state *awdl_state,
    omdrop_radio_ethernet_callback ethernet_callback,
    void *ethernet_context) {
  if (runtime == NULL) return;
  memset(runtime, 0, sizeof(*runtime));
  runtime->session = session;
  omdrop_radio_adapter_init(&runtime->adapter, &session->link, awdl_state,
                            ethernet_callback, ethernet_context);
  session->message_callback = runtime_message;
  session->message_context = runtime;
}

int omdrop_radio_runtime_start(struct omdrop_radio_runtime *runtime,
                               uint32_t host_features,
                               uint16_t max_frame_size) {
  if (runtime == NULL || runtime->session == NULL) return 0;
  return omdrop_radio_session_start(runtime->session, host_features,
                                    max_frame_size);
}

int omdrop_radio_runtime_set_channel(
    struct omdrop_radio_runtime *runtime,
    const struct omdrop_radio_channel *channel) {
  if (runtime == NULL || runtime->session == NULL) return 0;
  return omdrop_radio_session_set_channel(runtime->session, channel);
}

int omdrop_radio_runtime_start_rx(struct omdrop_radio_runtime *runtime) {
  if (runtime == NULL || runtime->session == NULL) return 0;
  return omdrop_radio_session_start_rx(runtime->session);
}

int omdrop_radio_runtime_stop_rx(struct omdrop_radio_runtime *runtime) {
  if (runtime == NULL || runtime->session == NULL) return 0;
  return omdrop_radio_session_stop_rx(runtime->session);
}

int omdrop_radio_runtime_pump(struct omdrop_radio_runtime *runtime) {
  if (runtime == NULL || runtime->session == NULL) return -1;
  return omdrop_radio_session_pump(runtime->session);
}

int omdrop_radio_runtime_send_raw(struct omdrop_radio_runtime *runtime,
                                  const uint8_t *frame, size_t length,
                                  uint16_t channel, uint8_t rate) {
  if (runtime == NULL) return 0;
  return omdrop_radio_adapter_send_raw(&runtime->adapter, frame, length,
                                       channel, rate);
}
