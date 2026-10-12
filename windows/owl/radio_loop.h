#ifndef OMDROP_RADIO_LOOP_H
#define OMDROP_RADIO_LOOP_H

#include "radio_session.h"

typedef int (*omdrop_radio_loop_stop_fn)(void *context);

/*
 * Runs the session until the stop callback returns non-zero or pump reports
 * an error. max_iterations is a test/safety bound; zero means unlimited.
 */
int omdrop_radio_session_run(struct omdrop_radio_session *session,
                             omdrop_radio_loop_stop_fn should_stop,
                             void *stop_context, size_t max_iterations);

#endif
