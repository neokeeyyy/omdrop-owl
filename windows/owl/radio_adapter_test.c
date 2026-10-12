#include "radio_adapter.h"

#include <assert.h>
#include <string.h>

struct capture {
  uint8_t data[OMDROP_RADIO_HEADER_SIZE + OMDROP_RADIO_MAX_PAYLOAD];
  size_t length;
};

static int capture_write(void *context, const uint8_t *data, size_t length) {
  struct capture *capture = context;
  memcpy(capture->data, data, length);
  capture->length = length;
  return 1;
}

int main(void) {
  struct capture capture = {0};
  struct omdrop_radio_link link;
  struct omdrop_radio_adapter adapter;
  uint8_t frame[3] = {0xaa, 0xbb, 0xcc};
  struct omdrop_radio_message message;
  struct omdrop_radio_frame decoded;

  omdrop_radio_link_init(&link, capture_write, &capture);
  omdrop_radio_adapter_init(&adapter, &link, NULL, NULL, NULL);
  assert(!omdrop_radio_adapter_send_raw(&adapter, NULL, sizeof(frame), 6, 1));
  assert(omdrop_radio_adapter_send_raw(&adapter, frame, sizeof(frame), 6, 1));
  assert(omdrop_radio_link_feed(&link, capture.data, capture.length,
                                &message) == OMDROP_RADIO_TRANSPORT_OK);
  assert(omdrop_radio_link_decode_rx(&message, &decoded) == 0);
  assert(message.type == OMDROP_RADIO_TX_FRAME);
  return 0;
}
