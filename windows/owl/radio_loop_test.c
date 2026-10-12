#include "radio_loop.h"

#include "fake_radio.h"

#include <assert.h>
#include <string.h>

struct loop_io {
  struct omdrop_fake_radio radio;
  int pumps;
};

static int write_fake(void *context, const uint8_t *data, size_t length) {
  struct loop_io *io = context;
  return omdrop_fake_radio_receive_host(&io->radio, data, length);
}

static int read_fake(void *context, uint8_t *data, size_t capacity,
                     size_t *length) {
  struct loop_io *io = context;
  io->pumps++;
  if (!omdrop_fake_radio_pop(&io->radio, data, capacity, length))
    *length = 0;
  return 1;
}

static int stop_after_first_pump(void *context) {
  const struct loop_io *io = context;
  return io->pumps >= 1;
}

int main(void) {
  struct loop_io io;
  struct omdrop_radio_session session;

  memset(&io, 0, sizeof(io));
  omdrop_fake_radio_init(&io.radio);
  omdrop_radio_session_init(&session, read_fake, write_fake, NULL, &io, NULL);
  assert(omdrop_radio_session_start(&session, OMDROP_RADIO_FEATURE_RAW_TX,
                                    OMDROP_RADIO_MAX_FRAME_SIZE));
  assert(omdrop_radio_session_run(&session, stop_after_first_pump, &io, 0) ==
         1);
  assert(session.has_capabilities);
  return 0;
}
