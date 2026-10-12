#ifndef OMDROP_WINDOWS_BACKEND_WORKER_H
#define OMDROP_WINDOWS_BACKEND_WORKER_H

#include <stdint.h>

int omdrop_windows_backend_worker_launch(uint32_t *pid);
int omdrop_windows_backend_worker_is_alive(uint32_t pid);
int omdrop_windows_backend_worker_run(const char *state_path,
                                      const char *stop_path);
int omdrop_windows_backend_worker_request_stop(const char *stop_path);
int omdrop_windows_backend_worker_stop_requested(const char *stop_path);
void omdrop_windows_backend_worker_clear_stop(const char *stop_path);

#endif
