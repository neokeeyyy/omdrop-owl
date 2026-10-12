#include "radio_adapter.h"

#include "wire.h"

#include <string.h>

#define OMDROP_RADIO_MAX_AMPDU 16u

void omdrop_radio_adapter_init(
    struct omdrop_radio_adapter *adapter, struct omdrop_radio_link *link,
    struct awdl_state *awdl_state,
    omdrop_radio_ethernet_callback ethernet_callback,
    void *ethernet_context) {
  if (adapter == NULL) return;
  memset(adapter, 0, sizeof(*adapter));
  adapter->link = link;
  adapter->awdl_state = awdl_state;
  adapter->ethernet_callback = ethernet_callback;
  adapter->ethernet_context = ethernet_context;
}

int omdrop_radio_adapter_send_raw(struct omdrop_radio_adapter *adapter,
                                  const uint8_t *frame, size_t length,
                                  uint16_t channel, uint8_t rate) {
  struct omdrop_radio_frame radio_frame;

  if (adapter == NULL || adapter->link == NULL || frame == NULL ||
      length == 0 || length > OMDROP_RADIO_MAX_FRAME_SIZE ||
      length > UINT16_MAX)
    return 0;

  radio_frame.channel = channel;
  radio_frame.rssi = 0;
  radio_frame.rate = rate;
  radio_frame.frame_length = (uint16_t)length;
  radio_frame.frame = frame;
  return omdrop_radio_link_send_frame(adapter->link, &radio_frame);
}

int omdrop_radio_adapter_receive(struct omdrop_radio_adapter *adapter,
                                 const struct omdrop_radio_message *message) {
  struct omdrop_radio_frame radio_frame;
  const struct buf *frame;
  struct buf *data_frames[OMDROP_RADIO_MAX_AMPDU];
  struct buf **data = data_frames;
  int result;

  if (adapter == NULL || adapter->awdl_state == NULL ||
      adapter->ethernet_callback == NULL ||
      !omdrop_radio_link_decode_rx(message, &radio_frame))
    return 0;

  frame = buf_new_const(radio_frame.frame, radio_frame.frame_length);
  if (frame == NULL) return 0;
  result = awdl_rx(frame, &data, &data_frames[OMDROP_RADIO_MAX_AMPDU],
                   adapter->awdl_state);
  while (data > data_frames) {
    struct buf *ethernet = *--data;
    if (result == RX_OK &&
        !adapter->ethernet_callback(adapter->ethernet_context,
                                     buf_data(ethernet), buf_len(ethernet))) {
      result = RX_UNEXPECTED_FORMAT;
    }
    buf_free(ethernet);
  }
  buf_free(frame);
  return result;
}
