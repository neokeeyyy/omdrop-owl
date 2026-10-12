#ifndef OMDROP_WINDOWS_BACKEND_RADIO_H
#define OMDROP_WINDOWS_BACKEND_RADIO_H

#include <stdint.h>

struct omdrop_backend_radio;

struct omdrop_backend_radio *omdrop_backend_radio_open(
    const char *port_name, int simulated);
int omdrop_backend_radio_start(struct omdrop_backend_radio *radio);
int omdrop_backend_radio_pump(struct omdrop_backend_radio *radio);
void omdrop_backend_radio_close(struct omdrop_backend_radio *radio);

#endif
