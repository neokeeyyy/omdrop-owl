#ifndef OMDROP_RADIO_LINK_H
#define OMDROP_RADIO_LINK_H

#include "radio_messages.h"
#include "serial_transport.h"

#include <stddef.h>
#include <stdint.h>

typedef int (*omdrop_radio_link_write_fn)(void *context,
                                          const uint8_t *data,
                                          size_t length);

struct omdrop_radio_link {
  struct omdrop_radio_parser parser;
  omdrop_radio_link_write_fn write;
  void *write_context;
  uint32_t next_sequence;
};

void omdrop_radio_link_init(struct omdrop_radio_link *link,
                            omdrop_radio_link_write_fn write,
                            void *write_context);

int omdrop_radio_link_feed(struct omdrop_radio_link *link,
                           const uint8_t *data, size_t length,
                           struct omdrop_radio_message *message);

int omdrop_radio_link_send(struct omdrop_radio_link *link, uint8_t type,
                           const uint8_t *payload, size_t payload_length);

int omdrop_radio_link_send_hello(struct omdrop_radio_link *link,
                                 const struct omdrop_radio_hello *hello);
int omdrop_radio_link_send_channel(
    struct omdrop_radio_link *link,
    const struct omdrop_radio_channel *channel);
int omdrop_radio_link_send_frame(struct omdrop_radio_link *link,
                                 const struct omdrop_radio_frame *frame);

int omdrop_radio_link_decode_rx(const struct omdrop_radio_message *message,
                                struct omdrop_radio_frame *frame);

#endif
