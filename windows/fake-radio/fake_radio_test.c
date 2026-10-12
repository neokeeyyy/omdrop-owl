#include "fake_radio.h"

#include <assert.h>
#include <string.h>

static size_t host_frame(uint8_t type, uint32_t sequence,
                         const uint8_t *payload, size_t payload_length,
                         uint8_t *output) {
  struct omdrop_radio_message message = {
      OMDROP_RADIO_PROTOCOL_VERSION, type, 0, sequence, payload,
      (uint32_t)payload_length};
  size_t length;
  assert(omdrop_radio_encode(&message, output,
                             OMDROP_RADIO_HEADER_SIZE +
                                 OMDROP_RADIO_MAX_PAYLOAD,
                             &length));
  return length;
}

int main(void) {
  struct omdrop_fake_radio radio;
  uint8_t wire[OMDROP_RADIO_HEADER_SIZE + OMDROP_RADIO_MAX_PAYLOAD];
  uint8_t response[sizeof(wire)];
  uint8_t payload[OMDROP_RADIO_MAX_PAYLOAD];
  size_t wire_length;
  size_t response_length;
  struct omdrop_radio_message message;
  struct omdrop_radio_hello hello = {OMDROP_RADIO_FEATURE_RAW_TX, 1200};
  struct omdrop_radio_channel channel = {6, 0, 0};
  struct omdrop_radio_frame frame = {6, -40, 1, 4,
                                     (const uint8_t *)"test"};
  struct omdrop_radio_status status;
  size_t payload_length;

  omdrop_fake_radio_init(&radio);

  assert(omdrop_radio_encode_hello(&hello, payload, sizeof(payload),
                                   &payload_length));
  wire_length = host_frame(OMDROP_RADIO_HELLO, 1, payload, payload_length, wire);
  assert(omdrop_fake_radio_receive_host(&radio, wire, 3));
  assert(omdrop_fake_radio_receive_host(&radio, wire + 3, wire_length - 3));
  assert(omdrop_fake_radio_pop(&radio, response, sizeof(response),
                               &response_length));
  assert(omdrop_radio_decode(response, response_length, &message, &wire_length));
  assert(message.type == OMDROP_RADIO_CAPABILITIES);

  assert(omdrop_radio_encode_channel(&channel, payload, sizeof(payload),
                                     &payload_length));
  wire_length = host_frame(OMDROP_RADIO_SET_CHANNEL, 2, payload, payload_length,
                           wire);
  assert(omdrop_fake_radio_receive_host(&radio, wire, wire_length));
  assert(omdrop_fake_radio_pop(&radio, response, sizeof(response),
                               &response_length));
  assert(omdrop_radio_decode(response, response_length, &message, &wire_length));
  assert(message.type == OMDROP_RADIO_STATUS);
  assert(omdrop_radio_decode_status(message.payload, message.payload_length,
                                    &status));
  assert(status.channel == 6);

  wire_length = host_frame(OMDROP_RADIO_START_RX, 3, NULL, 0, wire);
  assert(omdrop_fake_radio_receive_host(&radio, wire, wire_length));
  assert(omdrop_fake_radio_pop(&radio, response, sizeof(response),
                               &response_length));
  assert(omdrop_radio_decode(response, response_length, &message, &wire_length));
  assert(message.type == OMDROP_RADIO_STATUS);
  assert(omdrop_fake_radio_inject_rx(&radio, &frame));
  assert(omdrop_fake_radio_pop(&radio, response, sizeof(response),
                               &response_length));
  assert(omdrop_radio_decode(response, response_length, &message, &wire_length));
  assert(message.type == OMDROP_RADIO_RX_FRAME);

  assert(omdrop_radio_encode_frame(&frame, payload, sizeof(payload),
                                   &payload_length));
  wire_length = host_frame(OMDROP_RADIO_TX_FRAME, 4, payload, payload_length,
                           wire);
  assert(omdrop_fake_radio_receive_host(&radio, wire, wire_length));
  assert(omdrop_fake_radio_pop(&radio, response, sizeof(response),
                               &response_length));
  assert(omdrop_radio_decode(response, response_length, &message, &wire_length));
  assert(message.type == OMDROP_RADIO_TX_RESULT);
  return 0;
}
