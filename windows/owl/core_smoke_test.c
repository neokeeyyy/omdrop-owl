#include "channel.h"
#include "crc32.h"

#include <assert.h>
#include <stdint.h>

int main(void) {
  static const uint8_t sample[] = {0, 1, 2, 3};

  assert(awdl_chan_num(CHAN_OPCLASS_6, AWDL_CHAN_ENC_OPCLASS) == 6);
  assert(crc32(sample, sizeof(sample)) != 0);
  return 0;
}
