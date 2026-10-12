#include "backend_output.h"
#include "backend_radio.h"
#include "backend_state.h"
#include "backend_worker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *state_path(void) {
  const char *path = getenv("OMDROP_WINDOWS_STATE");
  return path != NULL && path[0] != '\0' ? path : "omdrop-windows.state";
}

static const char *stop_path(void) {
  const char *path = getenv("OMDROP_WINDOWS_STOP");
  return path != NULL && path[0] != '\0' ? path : "omdrop-windows.stop";
}

static int hardware_available(void) {
  const char *port = getenv("OMDROP_RADIO_PORT");
  const char *simulated = getenv("OMDROP_WINDOWS_SIMULATED");
  return (port != NULL && port[0] != '\0') ||
         (simulated != NULL && strcmp(simulated, "1") == 0);
}

static int simulated(void) {
  const char *value = getenv("OMDROP_WINDOWS_SIMULATED");
  return value != NULL && strcmp(value, "1") == 0;
}

static int load_state(struct omdrop_windows_backend_state *state) {
  omdrop_windows_backend_init(state);
  if (!omdrop_windows_backend_load(state_path(), state)) return 1;
  return omdrop_windows_backend_status(state, omdrop_windows_backend_now()) == 0;
}

static int save_state(const struct omdrop_windows_backend_state *state) {
  return omdrop_windows_backend_save(state_path(), state) ? 0 : 1;
}

static int usage(void) {
  fprintf(stderr, "usage: omdrop-windows-backend "
                  "{start <seconds>|stop|status [--json]|peers|probe [--json]}\n");
  return OMDROP_WINDOWS_BACKEND_USAGE;
}

static int start_radio(void) {
  struct omdrop_backend_radio *radio;
  const char *port = getenv("OMDROP_RADIO_PORT");
  if (!hardware_available()) return 0;
  radio = omdrop_backend_radio_open(port, simulated());
  if (radio == NULL || !omdrop_backend_radio_start(radio)) {
    if (radio != NULL) omdrop_backend_radio_close(radio);
    return 0;
  }
  omdrop_backend_radio_close(radio);
  return 1;
}

int main(int argc, char **argv) {
  struct omdrop_windows_backend_state state;
  int result;

  if (argc < 2) return usage();
  if (strcmp(argv[1], "--worker") == 0 && argc == 2)
    return omdrop_windows_backend_worker_run(state_path(), stop_path());
  load_state(&state);

  if (strcmp(argv[1], "start") == 0) {
    char *end;
    long seconds;
    if (argc != 3) return usage();
    seconds = strtol(argv[2], &end, 10);
    if (*argv[2] == '\0' || *end != '\0' || seconds < 0 || seconds > 86400)
      return usage();
    if (!start_radio()) {
      snprintf(state.reason, sizeof(state.reason),
               hardware_available() ? "radio handshake failed"
                                     : "no Windows radio is configured");
      (void)save_state(&state);
      fprintf(stderr, "omdrop-windows-backend: %s\n", state.reason);
      return OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
    }
    result = omdrop_windows_backend_start(&state, (int)seconds, 1);
    if (result == OMDROP_WINDOWS_BACKEND_OK ||
        (result == OMDROP_WINDOWS_BACKEND_EXTENDED &&
         !omdrop_windows_backend_worker_is_alive(state.worker_pid))) {
      omdrop_windows_backend_worker_clear_stop(stop_path());
      if (!omdrop_windows_backend_worker_launch(&state.worker_pid)) {
        state.visible = 0;
        state.worker_pid = 0;
        snprintf(state.reason, sizeof(state.reason),
                 "could not launch backend worker");
        result = OMDROP_WINDOWS_BACKEND_RETRYABLE;
      }
    }
    if (save_state(&state) != 0) return OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
    return result;
  }

  if (strcmp(argv[1], "stop") == 0 && argc == 2) {
    (void)omdrop_windows_backend_worker_request_stop(stop_path());
    result = omdrop_windows_backend_stop(&state);
    return save_state(&state) == 0 ? result : OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
  }

  if (strcmp(argv[1], "status") == 0 &&
      (argc == 2 || (argc == 3 && strcmp(argv[2], "--json") == 0))) {
    if (argc == 3)
      omdrop_windows_backend_print_json(&state);
    else
      printf("visible=%s\n", state.visible ? "true" : "false");
    return 0;
  }

  if (strcmp(argv[1], "peers") == 0 && argc == 2) return 0;
  if (strcmp(argv[1], "probe") == 0 &&
      (argc == 2 || (argc == 3 && strcmp(argv[2], "--json") == 0))) {
    int hardware = hardware_available();
    if (argc == 3) {
      printf("{\"backend\":\"owl-windows\",\"version\":\"%s\","
             "\"contract\":\"%s\",\"hardware\":%s,\"missing\":%s}\n",
             OMDROP_WINDOWS_BACKEND_VERSION, OMDROP_WINDOWS_BACKEND_CONTRACT,
             hardware ? "true" : "false",
             hardware ? "[]" : "[{\"id\":\"hardware\",\"say\":\"configure "
                         "OMDROP_RADIO_PORT or enable simulation\"}]");
    } else {
      printf("%s\n", hardware ? "ready" : "hardware|configure a radio");
    }
    return 0;
  }
  return usage();
}
