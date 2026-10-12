#ifndef OMDROP_RADIO_MESSAGES_H
#define OMDROP_RADIO_MESSAGES_H

#include "radio_protocol.h"

#define OMDROP_RADIO_MAC_SIZE 6u
#define OMDROP_RADIO_MAX_FRAME_SIZE (OMDROP_RADIO_MAX_PAYLOAD - 8u)

enum omdrop_radio_feature {
  OMDROP_RADIO_FEATURE_PROMISCUOUS_RX = 1u << 0,
  OMDROP_RADIO_FEATURE_RAW_TX = 1u << 1,
  OMDROP_RADIO_FEATURE_ACK = 1u << 2
};

enum omdrop_radio_tx_status {
  OMDROP_RADIO_TX_OK = 0,
  OMDROP_RADIO_TX_REJECTED = 1,
  OMDROP_RADIO_TX_BUSY = 2,
  OMDROP_RADIO_TX_ERROR = 3
};

enum omdrop_radio_state {
  OMDROP_RADIO_STATE_IDLE = 0,
  OMDROP_RADIO_STATE_RX = 1,
  OMDROP_RADIO_STATE_ERROR = 2
};

struct omdrop_radio_hello {
  uint32_t host_features;
  uint16_t max_frame_size;
};

struct omdrop_radio_capabilities {
  uint8_t hardware_major;
  uint8_t hardware_minor;
  uint16_t max_frame_size;
  uint32_t features;
  uint32_t channel_mask;
  uint8_t mac[OMDROP_RADIO_MAC_SIZE];
};

struct omdrop_radio_channel {
  uint16_t channel;
  uint8_t bandwidth;
  uint8_t flags;
};

struct omdrop_radio_frame {
  uint16_t channel;
  int8_t rssi;
  uint8_t rate;
  uint16_t frame_length;
  const uint8_t *frame;
};

struct omdrop_radio_tx_result {
  uint8_t status;
  uint8_t retries;
};

struct omdrop_radio_status {
  uint8_t state;
  uint16_t channel;
  uint8_t error;
  uint32_t rx_frames;
  uint32_t tx_frames;
};

int omdrop_radio_encode_hello(const struct omdrop_radio_hello *value,
                              uint8_t *payload, size_t capacity,
                              size_t *length);
int omdrop_radio_decode_hello(const uint8_t *payload, size_t length,
                              struct omdrop_radio_hello *value);

int omdrop_radio_encode_capabilities(
    const struct omdrop_radio_capabilities *value, uint8_t *payload,
    size_t capacity, size_t *length);
int omdrop_radio_decode_capabilities(
    const uint8_t *payload, size_t length,
    struct omdrop_radio_capabilities *value);

int omdrop_radio_encode_channel(const struct omdrop_radio_channel *value,
                                uint8_t *payload, size_t capacity,
                                size_t *length);
int omdrop_radio_decode_channel(const uint8_t *payload, size_t length,
                                struct omdrop_radio_channel *value);

int omdrop_radio_encode_frame(const struct omdrop_radio_frame *value,
                              uint8_t *payload, size_t capacity,
                              size_t *length);
int omdrop_radio_decode_frame(const uint8_t *payload, size_t length,
                              struct omdrop_radio_frame *value);

int omdrop_radio_encode_tx_result(const struct omdrop_radio_tx_result *value,
                                  uint8_t *payload, size_t capacity,
                                  size_t *length);
int omdrop_radio_decode_tx_result(const uint8_t *payload, size_t length,
                                  struct omdrop_radio_tx_result *value);

int omdrop_radio_encode_set_mac(const uint8_t mac[OMDROP_RADIO_MAC_SIZE],
                                uint8_t *payload, size_t capacity,
                                size_t *length);
int omdrop_radio_decode_set_mac(const uint8_t *payload, size_t length,
                                uint8_t mac[OMDROP_RADIO_MAC_SIZE]);

int omdrop_radio_encode_status(const struct omdrop_radio_status *value,
                               uint8_t *payload, size_t capacity,
                               size_t *length);
int omdrop_radio_decode_status(const uint8_t *payload, size_t length,
                               struct omdrop_radio_status *value);

int omdrop_radio_validate_payload(uint8_t type, const uint8_t *payload,
                                  size_t length);

#endif
