#include "backend_state.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

int64_t omdrop_windows_backend_now(void) {
  return (int64_t)time(NULL);
}

void omdrop_windows_backend_init(struct omdrop_windows_backend_state *state) {
  if (state == NULL) return;
  memset(state, 0, sizeof(*state));
}

int omdrop_windows_backend_start(struct omdrop_windows_backend_state *state,
                                 int seconds, int hardware_available) {
  if (state == NULL || seconds < 0) return OMDROP_WINDOWS_BACKEND_USAGE;
  if (!hardware_available) {
    snprintf(state->reason, sizeof(state->reason),
             "no Windows radio is configured");
    return OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
  }
  if (state->visible) {
    state->untimed = seconds == 0;
    state->expires_at = state->untimed ? 0 : omdrop_windows_backend_now() + seconds;
    return OMDROP_WINDOWS_BACKEND_EXTENDED;
  }
  state->visible = 1;
  state->untimed = seconds == 0;
  state->rx_proven = 0;
  state->expires_at = state->untimed ? 0 : omdrop_windows_backend_now() + seconds;
  state->reason[0] = '\0';
  return OMDROP_WINDOWS_BACKEND_OK;
}

int omdrop_windows_backend_stop(struct omdrop_windows_backend_state *state) {
  if (state == NULL) return OMDROP_WINDOWS_BACKEND_USAGE;
  state->visible = 0;
  state->untimed = 0;
  state->expires_at = 0;
  return OMDROP_WINDOWS_BACKEND_OK;
}

int omdrop_windows_backend_status(
    struct omdrop_windows_backend_state *state, int64_t now) {
  if (state == NULL) return OMDROP_WINDOWS_BACKEND_USAGE;
  if (state->visible && !state->untimed && now >= state->expires_at) {
    state->visible = 0;
    state->expires_at = 0;
    state->untimed = 0;
    snprintf(state->reason, sizeof(state->reason), "discoverability expired");
  }
  return OMDROP_WINDOWS_BACKEND_OK;
}

int omdrop_windows_backend_save(
    const char *path, const struct omdrop_windows_backend_state *state) {
  FILE *file;
  if (path == NULL || state == NULL) return 0;
  file = fopen(path, "wb");
  if (file == NULL) return 0;
  if (fwrite(state, sizeof(*state), 1, file) != 1) {
    fclose(file);
    return 0;
  }
  return fclose(file) == 0;
}

int omdrop_windows_backend_load(
    const char *path, struct omdrop_windows_backend_state *state) {
  FILE *file;
  if (path == NULL || state == NULL) return 0;
  file = fopen(path, "rb");
  if (file == NULL) return 0;
  if (fread(state, sizeof(*state), 1, file) != 1) {
    fclose(file);
    return 0;
  }
  return fclose(file) == 0;
}
