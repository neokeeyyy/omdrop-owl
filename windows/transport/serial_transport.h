#ifndef OMDROP_SERIAL_TRANSPORT_H
#define OMDROP_SERIAL_TRANSPORT_H

#include "radio_protocol.h"

#include <stddef.h>
#include <stdint.h>

#define OMDROP_RADIO_STREAM_BUFFER_SIZE \
  (2u * (OMDROP_RADIO_HEADER_SIZE + OMDROP_RADIO_MAX_PAYLOAD))

enum omdrop_radio_transport_result {
  OMDROP_RADIO_TRANSPORT_OK = 1,
  OMDROP_RADIO_TRANSPORT_NEED_MORE = 0,
  OMDROP_RADIO_TRANSPORT_ERROR = -1,
  OMDROP_RADIO_TRANSPORT_SEQUENCE_GAP = -2
};

struct omdrop_radio_parser {
  uint8_t buffer[OMDROP_RADIO_STREAM_BUFFER_SIZE];
  uint8_t frame[OMDROP_RADIO_HEADER_SIZE + OMDROP_RADIO_MAX_PAYLOAD];
  size_t length;
  uint32_t expected_sequence;
  int has_expected_sequence;
};

void omdrop_radio_parser_init(struct omdrop_radio_parser *parser);

/*
 * Feeds arbitrary transport bytes into the parser. One complete frame is
 * returned at a time. The returned message points into parser->buffer and is
 * valid until the next parser call.
 */
int omdrop_radio_parser_feed(struct omdrop_radio_parser *parser,
                             const uint8_t *data, size_t length,
                             struct omdrop_radio_message *message);

struct omdrop_radio_serial;

struct omdrop_radio_serial *omdrop_radio_serial_open(const char *port_name,
                                                      uint32_t baud_rate,
                                                      uint32_t timeout_ms);
int omdrop_radio_serial_read(struct omdrop_radio_serial *serial,
                             uint8_t *data, size_t capacity,
                             size_t *read_length);
int omdrop_radio_serial_write(struct omdrop_radio_serial *serial,
                              const uint8_t *data, size_t length);
void omdrop_radio_serial_close(struct omdrop_radio_serial *serial);

#endif
