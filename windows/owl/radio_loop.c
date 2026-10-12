#include "radio_loop.h"

int omdrop_radio_session_run(struct omdrop_radio_session *session,
                             omdrop_radio_loop_stop_fn should_stop,
                             void *stop_context, size_t max_iterations) {
  size_t iterations = 0;

  if (session == NULL || session->read == NULL) return -1;
  while (should_stop == NULL || !should_stop(stop_context)) {
    int result = omdrop_radio_session_pump(session);
    if (result < 0) return -1;
    if (max_iterations != 0 && ++iterations >= max_iterations) return 0;
  }
  return 1;
}
