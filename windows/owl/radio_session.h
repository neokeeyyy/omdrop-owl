#ifndef OMDROP_RADIO_SESSION_H
#define OMDROP_RADIO_SESSION_H

#include "radio_link.h"

#include <stddef.h>
#include <stdint.h>

typedef int (*omdrop_radio_session_write_fn)(void *context,
                                             const uint8_t *data,
                                             size_t length);
typedef int (*omdrop_radio_session_read_fn)(void *context, uint8_t *data,
                                            size_t capacity, size_t *length);
typedef int (*omdrop_radio_session_message_fn)(
    void *context, const struct omdrop_radio_message *message);

struct omdrop_radio_session {
  struct omdrop_radio_link link;
  omdrop_radio_session_read_fn read;
  omdrop_radio_session_message_fn message_callback;
  void *io_context;
  void *message_context;
  struct omdrop_radio_capabilities capabilities;
  struct omdrop_radio_status status;
  struct omdrop_radio_tx_result last_tx_result;
  int has_capabilities;
  int has_status;
  int has_tx_result;
};

struct omdrop_radio_serial_session {
  struct omdrop_radio_session session;
  struct omdrop_radio_serial *serial;
};

void omdrop_radio_session_init(struct omdrop_radio_session *session,
                               omdrop_radio_session_read_fn read,
                               omdrop_radio_session_write_fn write,
                               omdrop_radio_session_message_fn message_callback,
                               void *io_context,
                               void *message_context);

int omdrop_radio_session_start(struct omdrop_radio_session *session,
                               uint32_t host_features,
                               uint16_t max_frame_size);
int omdrop_radio_session_set_channel(
    struct omdrop_radio_session *session,
    const struct omdrop_radio_channel *channel);
int omdrop_radio_session_start_rx(struct omdrop_radio_session *session);
int omdrop_radio_session_stop_rx(struct omdrop_radio_session *session);
int omdrop_radio_session_pump(struct omdrop_radio_session *session);

int omdrop_radio_serial_session_open(struct omdrop_radio_serial_session *session,
                                     const char *port_name, uint32_t baud_rate,
                                     uint32_t timeout_ms,
                                     omdrop_radio_session_message_fn callback,
                                     void *message_context);
void omdrop_radio_serial_session_close(
    struct omdrop_radio_serial_session *session);
int omdrop_radio_serial_session_pump(
    struct omdrop_radio_serial_session *session);

#endif
