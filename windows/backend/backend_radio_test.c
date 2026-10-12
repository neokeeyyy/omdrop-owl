#include "backend_radio.h"

#include <assert.h>

int main(void) {
  struct omdrop_backend_radio *radio =
      omdrop_backend_radio_open(NULL, 1);
  assert(radio != NULL);
  assert(omdrop_backend_radio_start(radio));
  omdrop_backend_radio_close(radio);
  assert(omdrop_backend_radio_open(NULL, 0) == NULL);
  return 0;
}
