#ifndef OMDROP_RADIO_PROTOCOL_H
#define OMDROP_RADIO_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define OMDROP_RADIO_PROTOCOL_VERSION 1u
#define OMDROP_RADIO_MAGIC_0 'O'
#define OMDROP_RADIO_MAGIC_1 'W'
#define OMDROP_RADIO_MAGIC_2 'L'
#define OMDROP_RADIO_MAGIC_3 'R'
#define OMDROP_RADIO_HEADER_SIZE 20u
#define OMDROP_RADIO_MAX_PAYLOAD 4096u

enum omdrop_radio_message_type {
  OMDROP_RADIO_HELLO = 1,
  OMDROP_RADIO_CAPABILITIES = 2,
  OMDROP_RADIO_SET_CHANNEL = 3,
  OMDROP_RADIO_START_RX = 4,
  OMDROP_RADIO_STOP_RX = 5,
  OMDROP_RADIO_RX_FRAME = 6,
  OMDROP_RADIO_TX_FRAME = 7,
  OMDROP_RADIO_TX_RESULT = 8,
  OMDROP_RADIO_SET_MAC = 9,
  OMDROP_RADIO_STATUS = 10
};

int omdrop_radio_is_message_type(uint8_t type);

struct omdrop_radio_message {
  uint8_t version;
  uint8_t type;
  uint16_t flags;
  uint32_t sequence;
  const uint8_t *payload;
  uint32_t payload_length;
};

/*
 * A frame is encoded as:
 *
 *   magic[4], version[1], type[1], flags[2], sequence[4],
 *   payload_length[4], payload_crc32[4], payload[payload_length]
 *
 * All integer fields are little-endian. The CRC covers the payload only.
 * The caller owns the input and output buffers.
 */
int omdrop_radio_encode(const struct omdrop_radio_message *message,
                        uint8_t *output, size_t output_capacity,
                        size_t *output_length);

int omdrop_radio_decode(const uint8_t *input, size_t input_length,
                        struct omdrop_radio_message *message,
                        size_t *frame_length);

uint32_t omdrop_radio_crc32(const uint8_t *data, size_t length);

/* Sequence numbers are uint32 and wrap naturally after UINT32_MAX. */
int omdrop_radio_sequence_is_next(uint32_t previous, uint32_t current);

#endif
