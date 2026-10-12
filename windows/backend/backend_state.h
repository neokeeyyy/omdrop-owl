#ifndef OMDROP_WINDOWS_BACKEND_STATE_H
#define OMDROP_WINDOWS_BACKEND_STATE_H

#include <stdint.h>

#define OMDROP_WINDOWS_BACKEND_VERSION "0.1.0"
#define OMDROP_WINDOWS_BACKEND_CONTRACT "0.8.1"

enum omdrop_windows_backend_result {
  OMDROP_WINDOWS_BACKEND_OK = 0,
  OMDROP_WINDOWS_BACKEND_USAGE = 2,
  OMDROP_WINDOWS_BACKEND_RETRYABLE = 3,
  OMDROP_WINDOWS_BACKEND_UNAVAILABLE = 4,
  OMDROP_WINDOWS_BACKEND_EXTENDED = 5
};

struct omdrop_windows_backend_state {
  int visible;
  int untimed;
  int rx_proven;
  int64_t expires_at;
  uint32_t worker_pid;
  char reason[160];
};

int64_t omdrop_windows_backend_now(void);
void omdrop_windows_backend_init(struct omdrop_windows_backend_state *state);
int omdrop_windows_backend_start(struct omdrop_windows_backend_state *state,
                                 int seconds, int hardware_available);
int omdrop_windows_backend_stop(struct omdrop_windows_backend_state *state);
int omdrop_windows_backend_status(
    struct omdrop_windows_backend_state *state, int64_t now);
int omdrop_windows_backend_save(const char *path,
                                const struct omdrop_windows_backend_state *state);
int omdrop_windows_backend_load(const char *path,
                                struct omdrop_windows_backend_state *state);

#endif
