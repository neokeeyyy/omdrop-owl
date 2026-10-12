#include "backend_state.h"

#include <assert.h>

int main(void) {
  struct omdrop_windows_backend_state state;
  omdrop_windows_backend_init(&state);
  assert(omdrop_windows_backend_start(&state, 10, 0) ==
         OMDROP_WINDOWS_BACKEND_UNAVAILABLE);
  assert(!state.visible);
  assert(omdrop_windows_backend_start(&state, 10, 1) ==
         OMDROP_WINDOWS_BACKEND_OK);
  assert(state.visible && !state.untimed);
  assert(omdrop_windows_backend_start(&state, 0, 1) ==
         OMDROP_WINDOWS_BACKEND_EXTENDED);
  assert(state.untimed);
  assert(omdrop_windows_backend_stop(&state) == OMDROP_WINDOWS_BACKEND_OK);
  assert(!state.visible);
  assert(omdrop_windows_backend_start(&state, 1, 1) ==
         OMDROP_WINDOWS_BACKEND_OK);
  assert(omdrop_windows_backend_status(&state,
                                       state.expires_at) ==
         OMDROP_WINDOWS_BACKEND_OK);
  assert(!state.visible);
  return 0;
}
