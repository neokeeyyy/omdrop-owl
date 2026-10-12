#include "radio_session.h"

#include <string.h>

static int serial_write(void *context, const uint8_t *data, size_t length) {
  struct omdrop_radio_serial_session *serial_session = context;
  return omdrop_radio_serial_write(serial_session->serial, data, length);
}

static int serial_read(void *context, uint8_t *data, size_t capacity,
                       size_t *length) {
  struct omdrop_radio_serial_session *serial_session = context;
  return omdrop_radio_serial_read(serial_session->serial, data, capacity,
                                  length);
}

static int send_empty(struct omdrop_radio_session *session, uint8_t type) {
  return omdrop_radio_link_send(&session->link, type, NULL, 0);
}

void omdrop_radio_session_init(struct omdrop_radio_session *session,
                               omdrop_radio_session_read_fn read,
                               omdrop_radio_session_write_fn write,
                               omdrop_radio_session_message_fn message_callback,
                               void *io_context,
                               void *message_context) {
  if (session == NULL) return;
  memset(session, 0, sizeof(*session));
  session->read = read;
  session->message_callback = message_callback;
  session->io_context = io_context;
  session->message_context = message_context;
  omdrop_radio_link_init(&session->link, write, io_context);
}

int omdrop_radio_session_start(struct omdrop_radio_session *session,
                               uint32_t host_features,
                               uint16_t max_frame_size) {
  struct omdrop_radio_hello hello;
  if (session == NULL) return 0;
  hello.host_features = host_features;
  hello.max_frame_size = max_frame_size;
  session->has_capabilities = 0;
  return omdrop_radio_link_send_hello(&session->link, &hello);
}

int omdrop_radio_session_set_channel(
    struct omdrop_radio_session *session,
    const struct omdrop_radio_channel *channel) {
  if (session == NULL) return 0;
  return omdrop_radio_link_send_channel(&session->link, channel);
}

int omdrop_radio_session_start_rx(struct omdrop_radio_session *session) {
  if (session == NULL) return 0;
  return send_empty(session, OMDROP_RADIO_START_RX);
}

int omdrop_radio_session_stop_rx(struct omdrop_radio_session *session) {
  if (session == NULL) return 0;
  return send_empty(session, OMDROP_RADIO_STOP_RX);
}

static int handle_message(struct omdrop_radio_session *session,
                          const struct omdrop_radio_message *message) {
  switch (message->type) {
    case OMDROP_RADIO_CAPABILITIES:
      if (!omdrop_radio_decode_capabilities(
              message->payload, message->payload_length,
              &session->capabilities))
        return 0;
      session->has_capabilities = 1;
      return 1;
    case OMDROP_RADIO_STATUS:
      if (!omdrop_radio_decode_status(message->payload, message->payload_length,
                                      &session->status))
        return 0;
      session->has_status = 1;
      return 1;
    case OMDROP_RADIO_TX_RESULT:
      if (!omdrop_radio_decode_tx_result(message->payload,
                                         message->payload_length,
                                         &session->last_tx_result))
        return 0;
      session->has_tx_result = 1;
      return 1;
    case OMDROP_RADIO_RX_FRAME:
      return session->message_callback == NULL ||
             session->message_callback(session->message_context, message);
    default:
      return 0;
  }
}

int omdrop_radio_session_pump(struct omdrop_radio_session *session) {
  uint8_t buffer[OMDROP_RADIO_HEADER_SIZE + OMDROP_RADIO_MAX_PAYLOAD];
  size_t length;
  struct omdrop_radio_message message;
  int result;
  int handled = 0;

  if (session == NULL || session->read == NULL) return -1;
  if (!session->read(session->io_context, buffer, sizeof(buffer), &length))
    return -1;
  if (length == 0) return 0;

  result = omdrop_radio_link_feed(&session->link, buffer, length, &message);
  while (result == OMDROP_RADIO_TRANSPORT_OK ||
         result == OMDROP_RADIO_TRANSPORT_SEQUENCE_GAP) {
    if (!handle_message(session, &message)) return -1;
    handled++;
    result = omdrop_radio_link_feed(&session->link, NULL, 0, &message);
  }
  if (result == OMDROP_RADIO_TRANSPORT_NEED_MORE) return handled;
  return -1;
}

int omdrop_radio_serial_session_open(
    struct omdrop_radio_serial_session *serial_session, const char *port_name,
    uint32_t baud_rate, uint32_t timeout_ms,
    omdrop_radio_session_message_fn callback, void *message_context) {
  if (serial_session == NULL) return 0;
  memset(serial_session, 0, sizeof(*serial_session));
  serial_session->serial =
      omdrop_radio_serial_open(port_name, baud_rate, timeout_ms);
  if (serial_session->serial == NULL) return 0;
  omdrop_radio_session_init(&serial_session->session, serial_read, serial_write,
                            callback, serial_session, message_context);
  return 1;
}

void omdrop_radio_serial_session_close(
    struct omdrop_radio_serial_session *serial_session) {
  if (serial_session == NULL) return;
  omdrop_radio_serial_close(serial_session->serial);
  serial_session->serial = NULL;
  memset(&serial_session->session, 0, sizeof(serial_session->session));
}

int omdrop_radio_serial_session_pump(
    struct omdrop_radio_serial_session *serial_session) {
  if (serial_session == NULL || serial_session->serial == NULL) return -1;
  return omdrop_radio_session_pump(&serial_session->session);
}
