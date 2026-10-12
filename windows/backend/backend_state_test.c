#include "backend_state.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  struct omdrop_windows_backend_state state;
  struct omdrop_windows_backend_state loaded;
  unsigned char byte;
  FILE *file;
  const char *path = "backend-state-test.bin";

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

  assert(omdrop_windows_backend_start(&state, 60, 1) ==
         OMDROP_WINDOWS_BACKEND_OK);
  state.worker_pid = 1234;
  strcpy(state.reason, "persisted");
  assert(omdrop_windows_backend_save(path, &state));
  omdrop_windows_backend_init(&loaded);
  assert(omdrop_windows_backend_load(path, &loaded));
  assert(loaded.visible && loaded.worker_pid == 1234);
  assert(strcmp(loaded.reason, "persisted") == 0);

  file = fopen(path, "r+b");
  assert(file != NULL);
  assert(fread(&byte, sizeof(byte), 1, file) == 1);
  byte ^= 0xff;
  assert(fseek(file, 0, SEEK_SET) == 0);
  assert(fwrite(&byte, sizeof(byte), 1, file) == 1);
  assert(fclose(file) == 0);
  assert(!omdrop_windows_backend_load(path, &loaded));
  remove(path);
  return 0;
}
