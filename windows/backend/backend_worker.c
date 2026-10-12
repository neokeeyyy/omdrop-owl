#include "backend_worker.h"

#include "backend_radio.h"
#include "backend_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

static int simulated(void) {
  const char *value = getenv("OMDROP_WINDOWS_SIMULATED");
  return value != NULL && strcmp(value, "1") == 0;
}

static int hardware_available(void) {
  const char *port = getenv("OMDROP_RADIO_PORT");
  return (port != NULL && port[0] != '\0') || simulated();
}

int omdrop_windows_backend_worker_request_stop(const char *stop_path) {
  FILE *file;
  if (stop_path == NULL) return 0;
  file = fopen(stop_path, "wb");
  if (file == NULL) return 0;
  return fclose(file) == 0;
}

int omdrop_windows_backend_worker_stop_requested(const char *stop_path) {
  FILE *file;
  if (stop_path == NULL) return 0;
  file = fopen(stop_path, "rb");
  if (file == NULL) return 0;
  fclose(file);
  return 1;
}

void omdrop_windows_backend_worker_clear_stop(const char *stop_path) {
  if (stop_path != NULL) remove(stop_path);
}

#ifdef _WIN32
int omdrop_windows_backend_worker_launch(uint32_t *pid) {
  char executable[MAX_PATH];
  char command_line[MAX_PATH + 32];
  STARTUPINFOA startup = {sizeof(startup)};
  PROCESS_INFORMATION process = {0};
  DWORD length;

  if (pid == NULL) return 0;
  length = GetModuleFileNameA(NULL, executable, sizeof(executable));
  if (length == 0 || length >= sizeof(executable)) return 0;
  snprintf(command_line, sizeof(command_line), "\"%s\" --worker", executable);
  if (!CreateProcessA(NULL, command_line, NULL, NULL, FALSE, CREATE_NO_WINDOW,
                      NULL, NULL, &startup, &process))
    return 0;
  *pid = (uint32_t)process.dwProcessId;
  CloseHandle(process.hThread);
  CloseHandle(process.hProcess);
  return 1;
}

int omdrop_windows_backend_worker_is_alive(uint32_t pid) {
  HANDLE process;
  DWORD exit_code;
  if (pid == 0) return 0;
  process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (process == NULL) return 0;
  if (!GetExitCodeProcess(process, &exit_code)) {
    CloseHandle(process);
    return 0;
  }
  CloseHandle(process);
  return exit_code == STILL_ACTIVE;
}
#else
int omdrop_windows_backend_worker_launch(uint32_t *pid) {
  (void)pid;
  return 0;
}

int omdrop_windows_backend_worker_is_alive(uint32_t pid) {
  (void)pid;
  return 0;
}
#endif

int omdrop_windows_backend_worker_run(const char *state_path,
                                      const char *stop_path) {
  struct omdrop_backend_radio *radio = NULL;
  struct omdrop_windows_backend_state state;
  const char *port = getenv("OMDROP_RADIO_PORT");
  int is_simulated = simulated();

  if (state_path == NULL || stop_path == NULL || !hardware_available())
    return OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
  radio = omdrop_backend_radio_open(port, is_simulated);
  if (radio == NULL || !omdrop_backend_radio_start(radio)) {
    if (radio != NULL) omdrop_backend_radio_close(radio);
    omdrop_windows_backend_init(&state);
    (void)omdrop_windows_backend_load(state_path, &state);
    state.visible = 0;
    state.untimed = 0;
    state.expires_at = 0;
    state.worker_pid = 0;
    snprintf(state.reason, sizeof(state.reason), "radio worker failed");
    (void)omdrop_windows_backend_save(state_path, &state);
    return OMDROP_WINDOWS_BACKEND_UNAVAILABLE;
  }

  omdrop_windows_backend_init(&state);
  (void)omdrop_windows_backend_load(state_path, &state);
  while (!omdrop_windows_backend_worker_stop_requested(stop_path)) {
    omdrop_windows_backend_status(&state, omdrop_windows_backend_now());
    if (!state.visible) break;
    if (!is_simulated && omdrop_backend_radio_pump(radio) < 0) {
      snprintf(state.reason, sizeof(state.reason), "radio worker failed");
      break;
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
  if (omdrop_windows_backend_worker_stop_requested(stop_path)) {
    snprintf(state.reason, sizeof(state.reason), "stopped");
    omdrop_windows_backend_worker_clear_stop(stop_path);
  } else if (state.reason[0] == '\0') {
    snprintf(state.reason, sizeof(state.reason), "worker exited");
  }
  (void)omdrop_windows_backend_save(state_path, &state);
  return OMDROP_WINDOWS_BACKEND_OK;
}
