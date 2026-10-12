#include "radio_protocol.h"

#include <string.h>

int omdrop_radio_is_message_type(uint8_t type) {
  return type >= OMDROP_RADIO_HELLO && type <= OMDROP_RADIO_STATUS;
}

static uint16_t read_u16(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void write_u16(uint8_t *p, uint16_t value) {
  p[0] = (uint8_t)value;
  p[1] = (uint8_t)(value >> 8);
}

static void write_u32(uint8_t *p, uint32_t value) {
  p[0] = (uint8_t)value;
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
}

uint32_t omdrop_radio_crc32(const uint8_t *data, size_t length) {
  uint32_t crc = UINT32_C(0xffffffff);
  size_t i;

  for (i = 0; i < length; ++i) {
    uint32_t bit;
    crc ^= data[i];
    for (bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (uint32_t)-(int)(crc & 1));
    }
  }
  return ~crc;
}

int omdrop_radio_sequence_is_next(uint32_t previous, uint32_t current) {
  return current == previous + UINT32_C(1);
}

int omdrop_radio_encode(const struct omdrop_radio_message *message,
                        uint8_t *output, size_t output_capacity,
                        size_t *output_length) {
  size_t total;

  if (message == NULL || output == NULL || output_length == NULL ||
      message->version != OMDROP_RADIO_PROTOCOL_VERSION ||
      !omdrop_radio_is_message_type(message->type) ||
      message->payload_length > OMDROP_RADIO_MAX_PAYLOAD ||
      (message->payload_length != 0 && message->payload == NULL)) {
    return 0;
  }

  total = OMDROP_RADIO_HEADER_SIZE + message->payload_length;
  if (output_capacity < total) {
    return 0;
  }

  output[0] = OMDROP_RADIO_MAGIC_0;
  output[1] = OMDROP_RADIO_MAGIC_1;
  output[2] = OMDROP_RADIO_MAGIC_2;
  output[3] = OMDROP_RADIO_MAGIC_3;
  output[4] = message->version;
  output[5] = message->type;
  write_u16(output + 6, message->flags);
  write_u32(output + 8, message->sequence);
  write_u32(output + 12, message->payload_length);
  write_u32(output + 16, omdrop_radio_crc32(message->payload,
                                             message->payload_length));
  if (message->payload_length != 0) {
    memcpy(output + OMDROP_RADIO_HEADER_SIZE, message->payload,
           message->payload_length);
  }
  *output_length = total;
  return 1;
}

int omdrop_radio_decode(const uint8_t *input, size_t input_length,
                        struct omdrop_radio_message *message,
                        size_t *frame_length) {
  uint32_t payload_length;
  size_t total;

  if (input == NULL || message == NULL || frame_length == NULL ||
      input_length < OMDROP_RADIO_HEADER_SIZE ||
      input[0] != OMDROP_RADIO_MAGIC_0 || input[1] != OMDROP_RADIO_MAGIC_1 ||
      input[2] != OMDROP_RADIO_MAGIC_2 || input[3] != OMDROP_RADIO_MAGIC_3 ||
      input[4] != OMDROP_RADIO_PROTOCOL_VERSION ||
      !omdrop_radio_is_message_type(input[5])) {
    return 0;
  }

  payload_length = read_u32(input + 12);
  if (payload_length > OMDROP_RADIO_MAX_PAYLOAD) {
    return 0;
  }
  total = OMDROP_RADIO_HEADER_SIZE + payload_length;
  if (input_length < total ||
      omdrop_radio_crc32(input + OMDROP_RADIO_HEADER_SIZE, payload_length) !=
          read_u32(input + 16)) {
    return 0;
  }

  message->version = input[4];
  message->type = input[5];
  message->flags = read_u16(input + 6);
  message->sequence = read_u32(input + 8);
  message->payload = input + OMDROP_RADIO_HEADER_SIZE;
  message->payload_length = payload_length;
  *frame_length = total;
  return 1;
}
