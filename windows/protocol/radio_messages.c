#include "radio_messages.h"

#include <string.h>

static uint16_t get16(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t get32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put16(uint8_t *p, uint16_t value) {
  p[0] = (uint8_t)value;
  p[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t *p, uint32_t value) {
  p[0] = (uint8_t)value;
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
}

int omdrop_radio_encode_hello(const struct omdrop_radio_hello *value,
                              uint8_t *payload, size_t capacity,
                              size_t *length) {
  if (value == NULL || payload == NULL || length == NULL || capacity < 6 ||
      value->max_frame_size == 0 ||
      value->max_frame_size > OMDROP_RADIO_MAX_FRAME_SIZE) {
    return 0;
  }
  put32(payload, value->host_features);
  put16(payload + 4, value->max_frame_size);
  *length = 6;
  return 1;
}

int omdrop_radio_decode_hello(const uint8_t *payload, size_t length,
                              struct omdrop_radio_hello *value) {
  if (payload == NULL || value == NULL || length != 6) return 0;
  value->host_features = get32(payload);
  value->max_frame_size = get16(payload + 4);
  return value->max_frame_size != 0 &&
         value->max_frame_size <= OMDROP_RADIO_MAX_FRAME_SIZE;
}

int omdrop_radio_encode_capabilities(
    const struct omdrop_radio_capabilities *value, uint8_t *payload,
    size_t capacity, size_t *length) {
  if (value == NULL || payload == NULL || length == NULL || capacity < 18 ||
      value->max_frame_size == 0 ||
      value->max_frame_size > OMDROP_RADIO_MAX_FRAME_SIZE) {
    return 0;
  }
  payload[0] = value->hardware_major;
  payload[1] = value->hardware_minor;
  put16(payload + 2, value->max_frame_size);
  put32(payload + 4, value->features);
  put32(payload + 8, value->channel_mask);
  memcpy(payload + 12, value->mac, OMDROP_RADIO_MAC_SIZE);
  *length = 18;
  return 1;
}

int omdrop_radio_decode_capabilities(
    const uint8_t *payload, size_t length,
    struct omdrop_radio_capabilities *value) {
  if (payload == NULL || value == NULL || length != 18) return 0;
  value->hardware_major = payload[0];
  value->hardware_minor = payload[1];
  value->max_frame_size = get16(payload + 2);
  value->features = get32(payload + 4);
  value->channel_mask = get32(payload + 8);
  memcpy(value->mac, payload + 12, OMDROP_RADIO_MAC_SIZE);
  return value->max_frame_size != 0 &&
         value->max_frame_size <= OMDROP_RADIO_MAX_FRAME_SIZE;
}

int omdrop_radio_encode_channel(const struct omdrop_radio_channel *value,
                                uint8_t *payload, size_t capacity,
                                size_t *length) {
  if (value == NULL || payload == NULL || length == NULL || capacity < 4 ||
      value->channel == 0) return 0;
  put16(payload, value->channel);
  payload[2] = value->bandwidth;
  payload[3] = value->flags;
  *length = 4;
  return 1;
}

int omdrop_radio_decode_channel(const uint8_t *payload, size_t length,
                                struct omdrop_radio_channel *value) {
  if (payload == NULL || value == NULL || length != 4) return 0;
  value->channel = get16(payload);
  value->bandwidth = payload[2];
  value->flags = payload[3];
  return value->channel != 0;
}

static int encode_frame(const struct omdrop_radio_frame *value,
                        uint8_t *payload, size_t capacity, size_t *length) {
  size_t total;
  if (value == NULL || payload == NULL || length == NULL ||
      value->frame == NULL || value->frame_length == 0 ||
      value->frame_length > OMDROP_RADIO_MAX_FRAME_SIZE || value->channel == 0)
    return 0;
  total = 8u + value->frame_length;
  if (capacity < total) return 0;
  put16(payload, value->channel);
  payload[2] = (uint8_t)value->rssi;
  payload[3] = value->rate;
  put16(payload + 4, value->frame_length);
  put16(payload + 6, 0);
  memcpy(payload + 8, value->frame, value->frame_length);
  *length = total;
  return 1;
}

static int decode_frame(const uint8_t *payload, size_t length,
                        struct omdrop_radio_frame *value) {
  uint16_t frame_length;
  if (payload == NULL || value == NULL || length < 8) return 0;
  frame_length = get16(payload + 4);
  if (frame_length == 0 || frame_length > OMDROP_RADIO_MAX_FRAME_SIZE ||
      length != 8u + frame_length || get16(payload + 6) != 0 ||
      get16(payload) == 0)
    return 0;
  value->channel = get16(payload);
  value->rssi = (int8_t)payload[2];
  value->rate = payload[3];
  value->frame_length = frame_length;
  value->frame = payload + 8;
  return 1;
}

int omdrop_radio_encode_frame(const struct omdrop_radio_frame *value,
                              uint8_t *payload, size_t capacity,
                              size_t *length) {
  return encode_frame(value, payload, capacity, length);
}

int omdrop_radio_decode_frame(const uint8_t *payload, size_t length,
                              struct omdrop_radio_frame *value) {
  return decode_frame(payload, length, value);
}

int omdrop_radio_encode_tx_result(const struct omdrop_radio_tx_result *value,
                                  uint8_t *payload, size_t capacity,
                                  size_t *length) {
  if (value == NULL || payload == NULL || length == NULL || capacity < 4 ||
      value->status > OMDROP_RADIO_TX_ERROR)
    return 0;
  payload[0] = value->status;
  payload[1] = value->retries;
  payload[2] = payload[3] = 0;
  *length = 4;
  return 1;
}

int omdrop_radio_decode_tx_result(const uint8_t *payload, size_t length,
                                  struct omdrop_radio_tx_result *value) {
  if (payload == NULL || value == NULL || length != 4 ||
      payload[2] != 0 || payload[3] != 0 ||
      payload[0] > OMDROP_RADIO_TX_ERROR)
    return 0;
  value->status = payload[0];
  value->retries = payload[1];
  return 1;
}

int omdrop_radio_encode_set_mac(const uint8_t mac[OMDROP_RADIO_MAC_SIZE],
                                uint8_t *payload, size_t capacity,
                                size_t *length) {
  if (mac == NULL || payload == NULL || length == NULL || capacity < 6) return 0;
  memcpy(payload, mac, 6);
  *length = 6;
  return 1;
}

int omdrop_radio_decode_set_mac(const uint8_t *payload, size_t length,
                                uint8_t mac[OMDROP_RADIO_MAC_SIZE]) {
  if (payload == NULL || mac == NULL || length != 6) return 0;
  memcpy(mac, payload, 6);
  return 1;
}

int omdrop_radio_encode_status(const struct omdrop_radio_status *value,
                               uint8_t *payload, size_t capacity,
                               size_t *length) {
  if (value == NULL || payload == NULL || length == NULL || capacity < 12 ||
      value->state > OMDROP_RADIO_STATE_ERROR)
    return 0;
  payload[0] = value->state;
  put16(payload + 1, value->channel);
  payload[3] = value->error;
  put32(payload + 4, value->rx_frames);
  put32(payload + 8, value->tx_frames);
  *length = 12;
  return 1;
}

int omdrop_radio_decode_status(const uint8_t *payload, size_t length,
                               struct omdrop_radio_status *value) {
  if (payload == NULL || value == NULL || length != 12 ||
      payload[0] > OMDROP_RADIO_STATE_ERROR)
    return 0;
  value->state = payload[0];
  value->channel = get16(payload + 1);
  value->error = payload[3];
  value->rx_frames = get32(payload + 4);
  value->tx_frames = get32(payload + 8);
  return 1;
}

int omdrop_radio_validate_payload(uint8_t type, const uint8_t *payload,
                                  size_t length) {
  struct omdrop_radio_hello hello;
  struct omdrop_radio_capabilities capabilities;
  struct omdrop_radio_channel channel;
  struct omdrop_radio_frame frame;
  struct omdrop_radio_tx_result result;
  struct omdrop_radio_status status;
  uint8_t mac[OMDROP_RADIO_MAC_SIZE];

  switch (type) {
    case OMDROP_RADIO_HELLO:
      return omdrop_radio_decode_hello(payload, length, &hello);
    case OMDROP_RADIO_CAPABILITIES:
      return omdrop_radio_decode_capabilities(payload, length, &capabilities);
    case OMDROP_RADIO_SET_CHANNEL:
      return omdrop_radio_decode_channel(payload, length, &channel);
    case OMDROP_RADIO_START_RX:
    case OMDROP_RADIO_STOP_RX:
      return length == 0;
    case OMDROP_RADIO_RX_FRAME:
    case OMDROP_RADIO_TX_FRAME:
      return omdrop_radio_decode_frame(payload, length, &frame);
    case OMDROP_RADIO_TX_RESULT:
      return omdrop_radio_decode_tx_result(payload, length, &result);
    case OMDROP_RADIO_SET_MAC:
      return omdrop_radio_decode_set_mac(payload, length, mac);
    case OMDROP_RADIO_STATUS:
      return omdrop_radio_decode_status(payload, length, &status);
    default:
      return 0;
  }
}
