#ifndef OMDROP_WINDOWS_COMPAT_H
#define OMDROP_WINDOWS_COMPAT_H

#include <time.h>
#include <stdint.h>

static inline struct tm *omdrop_localtime_r(const time_t *time_value,
                                            struct tm *result) {
  return localtime_s(result, time_value) == 0 ? result : NULL;
}

#define localtime_r omdrop_localtime_r

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

#endif
