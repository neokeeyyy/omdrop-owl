#include "radio_session.h"

#include "fake_radio.h"

#include <assert.h>
#include <string.h>

struct fake_io {
  struct omdrop_fake_radio radio;
  int rx_messages;
};

static int write_fake(void *context, const uint8_t *data, size_t length) {
  struct fake_io *io = context;
  return omdrop_fake_radio_receive_host(&io->radio, data, length);
}

static int read_fake(void *context, uint8_t *data, size_t capacity,
                     size_t *length) {
  struct fake_io *io = context;
  if (!omdrop_fake_radio_pop(&io->radio, data, capacity, length)) {
    *length = 0;
  }
  return 1;
}

static int handle_message(void *context,
                          const struct omdrop_radio_message *message) {
  struct fake_io *io = context;
  if (message->type == OMDROP_RADIO_RX_FRAME) io->rx_messages++;
  return 1;
}

int main(void) {
  struct fake_io io;
  struct omdrop_radio_session session;
  struct omdrop_radio_serial_session serial_session;
  struct omdrop_radio_channel channel = {6, 20, 0};

  memset(&io, 0, sizeof(io));
  assert(!omdrop_radio_serial_session_open(&serial_session, "COM9999", 115200,
                                           20, NULL, NULL));
  assert(omdrop_radio_serial_session_pump(&serial_session) == -1);
  omdrop_radio_serial_session_close(&serial_session);
  omdrop_fake_radio_init(&io.radio);
  omdrop_radio_session_init(&session, read_fake, write_fake, handle_message,
                            &io, &io);
  assert(omdrop_radio_session_start(
      &session, OMDROP_RADIO_FEATURE_PROMISCUOUS_RX |
                    OMDROP_RADIO_FEATURE_RAW_TX,
      OMDROP_RADIO_MAX_FRAME_SIZE));
  assert(omdrop_radio_session_pump(&session) == 1);
  assert(session.has_capabilities);
  assert(session.capabilities.mac[0] == 0x02);
  assert(omdrop_radio_session_set_channel(&session, &channel));
  assert(omdrop_radio_session_pump(&session) == 1);
  assert(session.has_status && session.status.channel == 6);
  assert(omdrop_radio_session_start_rx(&session));
  assert(omdrop_radio_session_pump(&session) == 1);
  assert(session.status.state == OMDROP_RADIO_STATE_RX ||
         session.status.state == OMDROP_RADIO_STATE_IDLE);

  {
    uint8_t frame_data[] = {0xaa, 0xbb};
    struct omdrop_radio_frame frame = {6, -40, 1, sizeof(frame_data),
                                       frame_data};
    assert(omdrop_fake_radio_inject_rx(&io.radio, &frame));
    assert(omdrop_radio_session_pump(&session) == 1);
    assert(io.rx_messages == 1);
  }
  return 0;
}
