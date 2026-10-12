#include "backend_radio.h"

#include "fake_radio.h"
#include "radio_session.h"

#include <stdlib.h>
#include <string.h>

struct omdrop_backend_radio {
  int simulated;
  struct omdrop_radio_serial_session serial;
  struct omdrop_radio_session session;
  struct omdrop_fake_radio fake;
};

static int fake_write(void *context, const uint8_t *data, size_t length) {
  return omdrop_fake_radio_receive_host(context, data, length);
}

static int fake_read(void *context, uint8_t *data, size_t capacity,
                     size_t *length) {
  if (!omdrop_fake_radio_pop(context, data, capacity, length)) *length = 0;
  return 1;
}

static int simulated_start(struct omdrop_backend_radio *radio) {
  omdrop_fake_radio_init(&radio->fake);
  omdrop_radio_session_init(&radio->session, fake_read, fake_write, NULL,
                            &radio->fake, NULL);
  if (!omdrop_radio_session_start(
          &radio->session, OMDROP_RADIO_FEATURE_PROMISCUOUS_RX |
                              OMDROP_RADIO_FEATURE_RAW_TX,
          OMDROP_RADIO_MAX_FRAME_SIZE))
    return 0;
  return omdrop_radio_session_pump(&radio->session) > 0 &&
         radio->session.has_capabilities;
}

struct omdrop_backend_radio *omdrop_backend_radio_open(
    const char *port_name, int simulated) {
  struct omdrop_backend_radio *radio;
  if (!simulated && (port_name == NULL || port_name[0] == '\0')) return NULL;
  radio = calloc(1, sizeof(*radio));
  if (radio == NULL) return NULL;
  radio->simulated = simulated;
  if (!simulated &&
      !omdrop_radio_serial_session_open(&radio->serial, port_name, 115200, 250,
                                        NULL, NULL)) {
    free(radio);
    return NULL;
  }
  return radio;
}

int omdrop_backend_radio_start(struct omdrop_backend_radio *radio) {
  if (radio == NULL) return 0;
  if (radio->simulated) return simulated_start(radio);
  if (!omdrop_radio_session_start(
          &radio->serial.session, OMDROP_RADIO_FEATURE_PROMISCUOUS_RX |
                                      OMDROP_RADIO_FEATURE_RAW_TX,
          OMDROP_RADIO_MAX_FRAME_SIZE))
    return 0;
  return omdrop_radio_serial_session_pump(&radio->serial) > 0 &&
         radio->serial.session.has_capabilities;
}

int omdrop_backend_radio_pump(struct omdrop_backend_radio *radio) {
  if (radio == NULL) return -1;
  if (radio->simulated) return 0;
  return omdrop_radio_serial_session_pump(&radio->serial);
}

void omdrop_backend_radio_close(struct omdrop_backend_radio *radio) {
  if (radio == NULL) return;
  if (!radio->simulated)
    omdrop_radio_serial_session_close(&radio->serial);
  free(radio);
}
