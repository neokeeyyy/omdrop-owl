#ifndef OMDROP_FAKE_RADIO_H
#define OMDROP_FAKE_RADIO_H

#include "radio_messages.h"
#include "radio_protocol.h"
#include "serial_transport.h"

#include <stddef.h>
#include <stdint.h>

#define OMDROP_FAKE_RADIO_QUEUE_SIZE 8u

struct omdrop_fake_radio {
  struct omdrop_radio_parser parser;
  uint32_t next_sequence;
  uint32_t host_features;
  uint16_t host_max_frame_size;
  uint16_t channel;
  uint8_t state;
  uint8_t mac[OMDROP_RADIO_MAC_SIZE];
  uint32_t rx_frames;
  uint32_t tx_frames;
  uint8_t outbound[OMDROP_FAKE_RADIO_QUEUE_SIZE]
                 [OMDROP_RADIO_HEADER_SIZE + OMDROP_RADIO_MAX_PAYLOAD];
  size_t outbound_length[OMDROP_FAKE_RADIO_QUEUE_SIZE];
  size_t outbound_head;
  size_t outbound_count;
};

void omdrop_fake_radio_init(struct omdrop_fake_radio *radio);

int omdrop_fake_radio_receive_host(struct omdrop_fake_radio *radio,
                                   const uint8_t *data, size_t length);

int omdrop_fake_radio_pop(struct omdrop_fake_radio *radio, uint8_t *data,
                          size_t capacity, size_t *length);

int omdrop_fake_radio_inject_rx(struct omdrop_fake_radio *radio,
                                const struct omdrop_radio_frame *frame);

#endif
