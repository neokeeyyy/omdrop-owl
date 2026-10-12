#include "backend_state.h"
#include "backend_radio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

static const char *state_path(void) {
  const char *path = getenv("OMDROP_WINDOWS_STATE");
  return path != NULL && path[0] != '\0' ? path : "omdrop-windows.state";
}

static int load_state(struct omdrop_windows_backend_state *state) {
  omdrop_windows_backend_init(state);
  if (!omdrop_windows_backend_load(state_path(), state)) return 1;
  return omdrop_windows_backend_status(state, omdrop_windows_backend_now()) == 0;
}

static int save_state(const struct omdrop_windows_backend_state *state) {
  return omdrop_windows_backend_save(state_path(), state) ? 0 : 1;
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

static const char *stop_path(void) {
  const char *path = getenv("OMDROP_WINDOWS_STOP");
  return path != NULL && path[0] != '\0' ? path : "omdrop-windows.stop";
}

static void request_stop(void) {
  FILE *file = fopen(stop_path(), "wb");
  if (file != NULL) fclose(file);
}

static int stop_requested(void) {
  FILE *file = fopen(stop_path(), "rb");
  if (file == NULL) return 0;
  fclose(file);
  return 1;
}

static void clear_stop(void) {
  remove(stop_path());
}

#ifdef _WIN32
static int launch_worker(uint32_t *pid) {
  char executable[MAX_PATH];
  char command_line[MAX_PATH + 32];
  STARTUPINFOA startup = {sizeof(startup)};
  PROCESS_INFORMATION process = {0};
  DWORD length = GetModuleFileNameA(NULL, executable, sizeof(executable));
  if (length == 0 || length >= sizeof(executable)) return 0;
  snprintf(command_line, sizeof(command_line), "\"%s\" --worker", executable);
  if (!CreateProcessA(NULL, command_line, NULL, NULL, FALSE,
                      CREATE_NO_WINDOW, NULL, NULL, &startup, &process))
    return 0;
  *pid = (uint32_t)process.dwProcessId;
  CloseHandle(process.hThread);
  CloseHandle(process.hProcess);
  return 1;
}
#endif

static int run_worker(void) {
  struct omdrop_backend_radio *radio = NULL;
  struct omdrop_windows_backend_state state;
  const char *port = getenv("OMDROP_RADIO_PORT");
  int is_simulated = simulated();
  if (!hardware_available()) return OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
  radio = omdrop_backend_radio_open(port, is_simulated);
  if (radio == NULL || !omdrop_backend_radio_start(radio)) {
    if (radio != NULL) omdrop_backend_radio_close(radio);
    load_state(&state);
    state.visible = 0;
    state.untimed = 0;
    state.expires_at = 0;
    state.worker_pid = 0;
    snprintf(state.reason, sizeof(state.reason), "radio worker failed");
    (void)omdrop_windows_backend_save(state_path(), &state);
    return OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
  }
  load_state(&state);
  while (!stop_requested()) {
    int64_t now = omdrop_windows_backend_now();
    omdrop_windows_backend_status(&state, now);
    if (!state.visible) break;
    if (!is_simulated && omdrop_backend_radio_pump(radio) < 0)
      break;
    if (state.visible && !state.untimed) {
      (void)omdrop_windows_backend_save(state_path(), &state);
    }
#ifdef _WIN32
    Sleep(25);
#else
    break;
#endif
  }
  omdrop_backend_radio_close(radio);
  state.visible = 0;
  state.untimed = 0;
  state.expires_at = 0;
  state.worker_pid = 0;
  if (stop_requested()) {
    snprintf(state.reason, sizeof(state.reason), "stopped");
    clear_stop();
  }
  (void)omdrop_windows_backend_save(state_path(), &state);
  return 0;
}

static void print_json(const struct omdrop_windows_backend_state *state) {
  int64_t remaining = 0;
  char remaining_text[32];
  if (state->visible && !state->untimed)
    remaining = state->expires_at - omdrop_windows_backend_now();
  if (state->visible && !state->untimed) {
    snprintf(remaining_text, sizeof(remaining_text), "%lld",
             (long long)(remaining > 0 ? remaining : 0));
  } else {
    strcpy(remaining_text, "null");
  }
  printf("{\"visible\":%s,\"remaining\":%s,\"reason\":\"%s\","
         "\"rx_proven\":%s,\"worker_pid\":%u,\"peer_op\":\"\"}\n",
         state->visible ? "true" : "false",
         remaining_text,
         state->reason, state->rx_proven ? "true" : "false",
         state->worker_pid);
}

static int usage(void) {
  fprintf(stderr, "usage: omdrop-windows-backend "
                  "{start <seconds>|stop|status [--json]|peers|probe [--json]}\n");
  return OMDROP_WINDOWS_BACKEND_USAGE;
}

int main(int argc, char **argv) {
  struct omdrop_windows_backend_state state;
  int result;
  if (argc < 2) return usage();
  if (strcmp(argv[1], "--worker") == 0 && argc == 2) return run_worker();
  load_state(&state);

  if (strcmp(argv[1], "start") == 0) {
    char *end;
    long seconds;
    if (argc != 3) return usage();
    seconds = strtol(argv[2], &end, 10);
    if (*argv[2] == '\0' || *end != '\0' || seconds < 0 || seconds > 86400)
      return usage();
    {
      struct omdrop_backend_radio *radio = NULL;
      const char *port = getenv("OMDROP_RADIO_PORT");
      if (!hardware_available()) {
        snprintf(state.reason, sizeof(state.reason),
                 "no Windows radio is configured");
        return OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
      }
      clear_stop();
      radio = omdrop_backend_radio_open(port, simulated());
      if (radio == NULL || !omdrop_backend_radio_start(radio)) {
        if (radio != NULL) omdrop_backend_radio_close(radio);
        snprintf(state.reason, sizeof(state.reason),
                 "radio handshake failed");
        (void)save_state(&state);
        fprintf(stderr, "omdrop-windows-backend: %s\n", state.reason);
        return OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
      }
      omdrop_backend_radio_close(radio);
    }
    result = omdrop_windows_backend_start(&state, (int)seconds, 1);
    if (result == OMDROP_WINDOWS_BACKEND_OK) {
      if (save_state(&state) != 0)
        return OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
#ifdef _WIN32
      if (!launch_worker(&state.worker_pid)) {
        state.visible = 0;
        snprintf(state.reason, sizeof(state.reason),
                 "could not launch backend worker");
        result = OMDROP_WINDOWS_BACKEND_RETRYABLE;
      }
#else
      state.worker_pid = 0;
#endif
    }
    if (result == OMDROP_WINDOWS_BACKEND_UNAVAILABLE)
      fprintf(stderr, "omdrop-windows-backend: %s\n", state.reason);
    if (save_state(&state) != 0) return OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
    return result;
  }
  if (strcmp(argv[1], "stop") == 0 && argc == 2) {
    request_stop();
    result = omdrop_windows_backend_stop(&state);
    return save_state(&state) == 0 ? result : OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
  }
  if (strcmp(argv[1], "status") == 0 &&
      (argc == 2 || (argc == 3 && strcmp(argv[2], "--json") == 0))) {
    if (argc == 3) print_json(&state);
    else printf("visible=%s\n", state.visible ? "true" : "false");
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
