#include "fake_radio.h"

#include <string.h>

static size_t queue_index(const struct omdrop_fake_radio *radio) {
  return (radio->outbound_head + radio->outbound_count) %
         OMDROP_FAKE_RADIO_QUEUE_SIZE;
}

static int enqueue(struct omdrop_fake_radio *radio, uint8_t type,
                   const uint8_t *payload, size_t payload_length) {
  struct omdrop_radio_message message;
  size_t index;

  if (radio->outbound_count == OMDROP_FAKE_RADIO_QUEUE_SIZE ||
      payload_length > OMDROP_RADIO_MAX_PAYLOAD)
    return 0;
  index = queue_index(radio);
  message.version = OMDROP_RADIO_PROTOCOL_VERSION;
  message.type = type;
  message.flags = 0;
  message.sequence = radio->next_sequence++;
  message.payload = payload;
  message.payload_length = (uint32_t)payload_length;
  if (!omdrop_radio_encode(
          &message, radio->outbound[index],
          sizeof(radio->outbound[index]), &radio->outbound_length[index]))
    return 0;
  radio->outbound_count++;
  return 1;
}

static int enqueue_status(struct omdrop_fake_radio *radio) {
  struct omdrop_radio_status status = {
      radio->state, radio->channel, 0, radio->rx_frames, radio->tx_frames};
  uint8_t payload[12];
  size_t length;
  if (!omdrop_radio_encode_status(&status, payload, sizeof(payload), &length))
    return 0;
  return enqueue(radio, OMDROP_RADIO_STATUS, payload, length);
}

static int handle_message(struct omdrop_fake_radio *radio,
                          const struct omdrop_radio_message *message) {
  uint8_t payload[OMDROP_RADIO_MAX_PAYLOAD];
  size_t length;

  switch (message->type) {
    case OMDROP_RADIO_HELLO: {
      struct omdrop_radio_hello hello;
      struct omdrop_radio_capabilities capabilities = {
          1, 0, OMDROP_RADIO_MAX_FRAME_SIZE,
          OMDROP_RADIO_FEATURE_PROMISCUOUS_RX |
              OMDROP_RADIO_FEATURE_RAW_TX | OMDROP_RADIO_FEATURE_ACK,
          (UINT32_C(1) << 5), {0x02, 0x00, 0x00, 0x00, 0x00, 0x01}};
      if (!omdrop_radio_decode_hello(message->payload, message->payload_length,
                                     &hello))
        return 0;
      radio->host_features = hello.host_features;
      radio->host_max_frame_size = hello.max_frame_size;
      if (!omdrop_radio_encode_capabilities(&capabilities, payload,
                                            sizeof(payload), &length))
        return 0;
      return enqueue(radio, OMDROP_RADIO_CAPABILITIES, payload, length);
    }
    case OMDROP_RADIO_SET_CHANNEL: {
      struct omdrop_radio_channel channel;
      if (!omdrop_radio_decode_channel(message->payload, message->payload_length,
                                       &channel))
        return 0;
      radio->channel = channel.channel;
      return enqueue_status(radio);
    }
    case OMDROP_RADIO_START_RX:
      if (message->payload_length != 0) return 0;
      radio->state = OMDROP_RADIO_STATE_RX;
      return enqueue_status(radio);
    case OMDROP_RADIO_STOP_RX:
      if (message->payload_length != 0) return 0;
      radio->state = OMDROP_RADIO_STATE_IDLE;
      return enqueue_status(radio);
    case OMDROP_RADIO_SET_MAC:
      if (!omdrop_radio_decode_set_mac(message->payload, message->payload_length,
                                       radio->mac))
        return 0;
      return enqueue_status(radio);
    case OMDROP_RADIO_TX_FRAME: {
      struct omdrop_radio_frame frame;
      struct omdrop_radio_tx_result result = {OMDROP_RADIO_TX_OK, 0};
      if (!omdrop_radio_decode_frame(message->payload, message->payload_length,
                                     &frame) ||
          frame.frame_length > radio->host_max_frame_size)
        result.status = OMDROP_RADIO_TX_REJECTED;
      else
        radio->tx_frames++;
      if (!omdrop_radio_encode_tx_result(&result, payload, sizeof(payload),
                                         &length))
        return 0;
      return enqueue(radio, OMDROP_RADIO_TX_RESULT, payload, length);
    }
    default:
      return 0;
  }
}

void omdrop_fake_radio_init(struct omdrop_fake_radio *radio) {
  if (radio == NULL) return;
  memset(radio, 0, sizeof(*radio));
  omdrop_radio_parser_init(&radio->parser);
  radio->next_sequence = 1;
  radio->channel = 6;
  radio->state = OMDROP_RADIO_STATE_IDLE;
  memcpy(radio->mac, (uint8_t[]){0x02, 0, 0, 0, 0, 1},
         OMDROP_RADIO_MAC_SIZE);
}

int omdrop_fake_radio_receive_host(struct omdrop_fake_radio *radio,
                                   const uint8_t *data, size_t length) {
  struct omdrop_radio_message message;
  int result;

  if (radio == NULL) return 0;
  result = omdrop_radio_parser_feed(&radio->parser, data, length, &message);
  while (result == OMDROP_RADIO_TRANSPORT_OK) {
    if (!handle_message(radio, &message)) return 0;
    result = omdrop_radio_parser_feed(&radio->parser, NULL, 0, &message);
  }
  return result == OMDROP_RADIO_TRANSPORT_NEED_MORE;
}

int omdrop_fake_radio_pop(struct omdrop_fake_radio *radio, uint8_t *data,
                          size_t capacity, size_t *length) {
  size_t index;
  if (radio == NULL || data == NULL || length == NULL ||
      radio->outbound_count == 0)
    return 0;
  index = radio->outbound_head;
  if (capacity < radio->outbound_length[index]) return 0;
  memcpy(data, radio->outbound[index], radio->outbound_length[index]);
  *length = radio->outbound_length[index];
  radio->outbound_head = (radio->outbound_head + 1) %
                         OMDROP_FAKE_RADIO_QUEUE_SIZE;
  radio->outbound_count--;
  return 1;
}

int omdrop_fake_radio_inject_rx(struct omdrop_fake_radio *radio,
                                const struct omdrop_radio_frame *frame) {
  uint8_t payload[OMDROP_RADIO_MAX_PAYLOAD];
  size_t length;
  if (radio == NULL || radio->state != OMDROP_RADIO_STATE_RX ||
      !omdrop_radio_encode_frame(frame, payload, sizeof(payload), &length))
    return 0;
  radio->rx_frames++;
  return enqueue(radio, OMDROP_RADIO_RX_FRAME, payload, length);
}
