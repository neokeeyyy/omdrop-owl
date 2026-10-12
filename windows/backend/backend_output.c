#include "backend_output.h"

#include <stdio.h>
#include <string.h>

void omdrop_windows_backend_print_json(
    const struct omdrop_windows_backend_state *state) {
  int64_t remaining = 0;
  char remaining_text[32];
  if (state == NULL) return;
  if (state->visible && !state->untimed)
    remaining = state->expires_at - omdrop_windows_backend_now();
  if (state->visible && !state->untimed)
    snprintf(remaining_text, sizeof(remaining_text), "%lld",
             (long long)(remaining > 0 ? remaining : 0));
  else
    strcpy(remaining_text, "null");
  printf("{\"visible\":%s,\"remaining\":%s,\"reason\":\"%s\","
         "\"rx_proven\":%s,\"worker_pid\":%u,\"peer_op\":\"\"}\n",
         state->visible ? "true" : "false",
         remaining_text,
         state->reason, state->rx_proven ? "true" : "false",
         state->worker_pid);
}
