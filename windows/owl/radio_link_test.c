#include "radio_link.h"

#include <assert.h>
#include <string.h>

struct capture {
  uint8_t data[OMDROP_RADIO_HEADER_SIZE + OMDROP_RADIO_MAX_PAYLOAD];
  size_t length;
};

static int capture_write(void *context, const uint8_t *data, size_t length) {
  struct capture *capture = context;
  assert(length <= sizeof(capture->data));
  memcpy(capture->data, data, length);
  capture->length = length;
  return 1;
}

int main(void) {
  struct capture capture = {0};
  struct omdrop_radio_link link;
  struct omdrop_radio_message message;
  struct omdrop_radio_hello hello = {
      .host_features = OMDROP_RADIO_FEATURE_RAW_TX,
      .max_frame_size = OMDROP_RADIO_MAX_FRAME_SIZE};
  struct omdrop_radio_hello decoded;
  struct omdrop_radio_frame tx = {
      .channel = 6, .rssi = 0, .rate = 1, .frame_length = 3};
  uint8_t frame_data[] = {1, 2, 3};
  int result;

  tx.frame = frame_data;
  omdrop_radio_link_init(&link, capture_write, &capture);
  assert(omdrop_radio_link_send_hello(&link, &hello));
  assert(capture.length != 0);

  result = omdrop_radio_link_feed(&link, capture.data, capture.length,
                                  &message);
  assert(result == OMDROP_RADIO_TRANSPORT_OK);
  assert(message.type == OMDROP_RADIO_HELLO);
  assert(omdrop_radio_decode_hello(message.payload, message.payload_length,
                                   &decoded));
  assert(decoded.host_features == hello.host_features);

  assert(omdrop_radio_link_send_frame(&link, &tx));
  assert(omdrop_radio_link_feed(&link, capture.data, capture.length,
                                &message) == OMDROP_RADIO_TRANSPORT_OK);
  assert(message.type == OMDROP_RADIO_TX_FRAME);
  return 0;
}
