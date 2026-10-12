#include "radio_runtime.h"

#include "fake_radio.h"
#include "state.h"
#include "tx.h"

#include <assert.h>
#include <string.h>

struct fake_io {
  struct omdrop_fake_radio radio;
  int ethernet_frames;
};

static int write_fake(void *context, const uint8_t *data, size_t length) {
  struct fake_io *io = context;
  return omdrop_fake_radio_receive_host(&io->radio, data, length);
}

static int read_fake(void *context, uint8_t *data, size_t capacity,
                     size_t *length) {
  struct fake_io *io = context;
  if (!omdrop_fake_radio_pop(&io->radio, data, capacity, length))
    *length = 0;
  return 1;
}

static int capture_ethernet(void *context, const uint8_t *data,
                            size_t length) {
  struct fake_io *io = context;
  (void)data;
  if (length == 0) return 0;
  io->ethernet_frames++;
  return 1;
}

int main(void) {
  struct fake_io io;
  struct omdrop_radio_session session;
  struct omdrop_radio_runtime runtime;
  struct omdrop_radio_channel channel = {6, 20, 0};
  uint8_t raw_frame[] = {0, 1, 2, 3};
  struct awdl_state receiver_state;
  struct awdl_state sender_state;
  struct ieee80211_state sender_ieee;
  struct ether_addr receiver_mac = {{0x02, 0, 0, 0, 0, 2}};
  struct ether_addr sender_mac = {{0x02, 0, 0, 0, 0, 3}};
  uint8_t action_frame[4096];
  int action_length;

  memset(&io, 0, sizeof(io));
  omdrop_fake_radio_init(&io.radio);
  omdrop_radio_session_init(&session, read_fake, write_fake, NULL, &io, NULL);
  awdl_init_state(&receiver_state, "windows-test", &receiver_mac,
                  CHAN_OPCLASS_6, 1);
  awdl_init_state(&sender_state, "radio-test", &sender_mac, CHAN_OPCLASS_6, 1);
  ieee80211_init_state(&sender_ieee);
  omdrop_radio_runtime_init(&runtime, &session, &receiver_state,
                            capture_ethernet, &io);

  assert(omdrop_radio_runtime_start(&runtime, OMDROP_RADIO_FEATURE_RAW_TX,
                                    OMDROP_RADIO_MAX_FRAME_SIZE));
  assert(omdrop_radio_runtime_pump(&runtime) == 1);
  assert(session.has_capabilities);
  assert(omdrop_radio_runtime_set_channel(&runtime, &channel));
  assert(omdrop_radio_runtime_pump(&runtime) == 1);
  assert(omdrop_radio_runtime_start_rx(&runtime));
  assert(omdrop_radio_runtime_pump(&runtime) == 1);
  assert(omdrop_radio_runtime_send_raw(&runtime, raw_frame,
                                       sizeof(raw_frame), 6, 1));
  assert(omdrop_radio_runtime_pump(&runtime) == 1);
  assert(session.has_tx_result);
  assert(session.last_tx_result.status == OMDROP_RADIO_TX_OK);

  action_length = awdl_init_full_action_frame(
      action_frame, &sender_state, &sender_ieee, AWDL_ACTION_PSF);
  assert(action_length > 0);
  assert((size_t)action_length <= OMDROP_RADIO_MAX_FRAME_SIZE);
  {
    struct omdrop_radio_frame rx = {6, -40, 1, (uint16_t)action_length,
                                    action_frame};
    assert(omdrop_fake_radio_inject_rx(&io.radio, &rx));
    assert(omdrop_radio_runtime_pump(&runtime) == 1);
    assert(receiver_state.stats.rx_action > 0);
  }
  return 0;
}
