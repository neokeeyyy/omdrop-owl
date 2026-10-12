#include "serial_transport.h"

#include <assert.h>
#include <string.h>

static size_t encode(uint32_t sequence, uint8_t *buffer) {
  static const uint8_t payload[] = {1, 2, 3};
  struct omdrop_radio_message message = {
      OMDROP_RADIO_PROTOCOL_VERSION,
      OMDROP_RADIO_STATUS,
      0,
      sequence,
      payload,
      sizeof(payload),
  };
  size_t length = 0;
  assert(omdrop_radio_encode(&message, buffer,
                             OMDROP_RADIO_STREAM_BUFFER_SIZE, &length));
  return length;
}

int main(void) {
  struct omdrop_radio_parser parser;
  struct omdrop_radio_message message;
  uint8_t first[OMDROP_RADIO_STREAM_BUFFER_SIZE];
  uint8_t second[OMDROP_RADIO_STREAM_BUFFER_SIZE];
  uint8_t joined[OMDROP_RADIO_STREAM_BUFFER_SIZE * 2];
  size_t first_length = encode(10, first);
  size_t second_length = encode(11, second);

  omdrop_radio_parser_init(&parser);
  assert(omdrop_radio_parser_feed(&parser, first, 5, &message) ==
         OMDROP_RADIO_TRANSPORT_NEED_MORE);
  assert(omdrop_radio_parser_feed(&parser, first + 5, first_length - 5,
                                  &message) == OMDROP_RADIO_TRANSPORT_OK);
  assert(message.sequence == 10);

  omdrop_radio_parser_init(&parser);
  memcpy(joined, first, first_length);
  memcpy(joined + first_length, second, second_length);
  assert(omdrop_radio_parser_feed(&parser, joined, first_length + second_length,
                                  &message) == OMDROP_RADIO_TRANSPORT_OK);
  assert(message.sequence == 10);
  assert(omdrop_radio_parser_feed(&parser, NULL, 0, &message) ==
         OMDROP_RADIO_TRANSPORT_OK);
  assert(message.sequence == 11);

  first[OMDROP_RADIO_HEADER_SIZE] ^= 0xff;
  omdrop_radio_parser_init(&parser);
  assert(omdrop_radio_parser_feed(&parser, first, first_length, &message) ==
         OMDROP_RADIO_TRANSPORT_NEED_MORE);
  assert(omdrop_radio_parser_feed(&parser, second, second_length, &message) ==
         OMDROP_RADIO_TRANSPORT_OK);
  assert(message.sequence == 11);

  omdrop_radio_parser_init(&parser);
  first_length = encode(10, first);
  second_length = encode(12, second);
  assert(omdrop_radio_parser_feed(&parser, first, first_length, &message) ==
         OMDROP_RADIO_TRANSPORT_OK);
  assert(message.sequence == 10);
  assert(omdrop_radio_parser_feed(&parser, second, second_length, &message) ==
         OMDROP_RADIO_TRANSPORT_SEQUENCE_GAP);
  assert(message.sequence == 12);
  return 0;
}
