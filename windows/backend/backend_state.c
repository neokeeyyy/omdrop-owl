#include "backend_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define STATE_MAGIC UINT32_C(0x4f4d5357)
#define STATE_VERSION 1u

struct state_disk {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  uint32_t checksum;
  int32_t visible;
  int32_t untimed;
  int32_t rx_proven;
  int64_t expires_at;
  uint32_t worker_pid;
  char reason[160];
};

static uint32_t checksum(const struct state_disk *disk) {
  const unsigned char *bytes = (const unsigned char *)disk;
  uint32_t value = UINT32_C(2166136261);
  size_t i;
  for (i = offsetof(struct state_disk, visible); i < sizeof(*disk); ++i) {
    value ^= bytes[i];
    value *= UINT32_C(16777619);
  }
  return value;
}

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
  char *temporary;
  size_t path_length;
  struct state_disk disk;
  if (path == NULL || state == NULL) return 0;
  path_length = strlen(path) + 5;
  temporary = malloc(path_length);
  if (temporary == NULL) return 0;
  snprintf(temporary, path_length, "%s.tmp", path);
  memset(&disk, 0, sizeof(disk));
  disk.magic = STATE_MAGIC;
  disk.version = STATE_VERSION;
  disk.size = (uint16_t)sizeof(disk);
  disk.visible = state->visible;
  disk.untimed = state->untimed;
  disk.rx_proven = state->rx_proven;
  disk.expires_at = state->expires_at;
  disk.worker_pid = state->worker_pid;
  memcpy(disk.reason, state->reason, sizeof(disk.reason));
  disk.checksum = checksum(&disk);
  file = fopen(temporary, "wb");
  if (file == NULL) {
    free(temporary);
    return 0;
  }
  if (fwrite(&disk, sizeof(disk), 1, file) != 1 ||
      fclose(file) != 0) {
    remove(temporary);
    free(temporary);
    return 0;
  }
#ifdef _WIN32
  if (!MoveFileExA(temporary, path, MOVEFILE_REPLACE_EXISTING)) {
    remove(temporary);
    free(temporary);
    return 0;
  }
#else
  if (rename(temporary, path) != 0) {
    remove(temporary);
    free(temporary);
    return 0;
  }
#endif
  free(temporary);
  return 1;
}

int omdrop_windows_backend_load(
    const char *path, struct omdrop_windows_backend_state *state) {
  FILE *file;
  struct state_disk disk;
  if (path == NULL || state == NULL) return 0;
  file = fopen(path, "rb");
  if (file == NULL) return 0;
  if (fread(&disk, sizeof(disk), 1, file) != 1) {
    fclose(file);
    return 0;
  }
  if (fclose(file) != 0) return 0;
  if (disk.magic != STATE_MAGIC || disk.version != STATE_VERSION ||
      disk.size != sizeof(disk) || disk.checksum != checksum(&disk))
    return 0;
  state->visible = disk.visible;
  state->untimed = disk.untimed;
  state->rx_proven = disk.rx_proven;
  state->expires_at = disk.expires_at;
  state->worker_pid = disk.worker_pid;
  memcpy(state->reason, disk.reason, sizeof(state->reason));
  state->reason[sizeof(state->reason) - 1] = '\0';
  return 1;
}
