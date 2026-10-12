#include "radio_link.h"

#include <string.h>

void omdrop_radio_link_init(struct omdrop_radio_link *link,
                            omdrop_radio_link_write_fn write,
                            void *write_context) {
  if (link == NULL) return;
  memset(link, 0, sizeof(*link));
  omdrop_radio_parser_init(&link->parser);
  link->write = write;
  link->write_context = write_context;
}

int omdrop_radio_link_feed(struct omdrop_radio_link *link,
                           const uint8_t *data, size_t length,
                           struct omdrop_radio_message *message) {
  if (link == NULL) return OMDROP_RADIO_TRANSPORT_ERROR;
  return omdrop_radio_parser_feed(&link->parser, data, length, message);
}

int omdrop_radio_link_send(struct omdrop_radio_link *link, uint8_t type,
                           const uint8_t *payload, size_t payload_length) {
  uint8_t frame[OMDROP_RADIO_HEADER_SIZE + OMDROP_RADIO_MAX_PAYLOAD];
  struct omdrop_radio_message message;
  size_t frame_length;

  if (link == NULL || link->write == NULL ||
      payload_length > OMDROP_RADIO_MAX_PAYLOAD ||
      (payload_length != 0 && payload == NULL) ||
      !omdrop_radio_is_message_type(type)) {
    return 0;
  }

  memset(&message, 0, sizeof(message));
  message.version = OMDROP_RADIO_PROTOCOL_VERSION;
  message.type = type;
  message.sequence = link->next_sequence++;
  message.payload = payload;
  message.payload_length = (uint32_t)payload_length;
  if (!omdrop_radio_encode(&message, frame, sizeof(frame), &frame_length))
    return 0;
  return link->write(link->write_context, frame, frame_length);
}

int omdrop_radio_link_send_hello(struct omdrop_radio_link *link,
                                 const struct omdrop_radio_hello *hello) {
  uint8_t payload[16];
  size_t length;
  if (hello == NULL ||
      !omdrop_radio_encode_hello(hello, payload, sizeof(payload), &length))
    return 0;
  return omdrop_radio_link_send(link, OMDROP_RADIO_HELLO, payload, length);
}

int omdrop_radio_link_send_channel(
    struct omdrop_radio_link *link,
    const struct omdrop_radio_channel *channel) {
  uint8_t payload[16];
  size_t length;
  if (channel == NULL ||
      !omdrop_radio_encode_channel(channel, payload, sizeof(payload), &length))
    return 0;
  return omdrop_radio_link_send(link, OMDROP_RADIO_SET_CHANNEL, payload,
                                length);
}

int omdrop_radio_link_send_frame(struct omdrop_radio_link *link,
                                 const struct omdrop_radio_frame *frame) {
  uint8_t payload[OMDROP_RADIO_MAX_PAYLOAD];
  size_t length;
  if (frame == NULL ||
      !omdrop_radio_encode_frame(frame, payload, sizeof(payload), &length))
    return 0;
  return omdrop_radio_link_send(link, OMDROP_RADIO_TX_FRAME, payload, length);
}

int omdrop_radio_link_decode_rx(const struct omdrop_radio_message *message,
                                struct omdrop_radio_frame *frame) {
  if (message == NULL || frame == NULL ||
      message->type != OMDROP_RADIO_RX_FRAME)
    return 0;
  return omdrop_radio_decode_frame(message->payload, message->payload_length,
                                   frame);
}
