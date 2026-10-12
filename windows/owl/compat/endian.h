#ifndef OMDROP_WINDOWS_ENDIAN_H
#define OMDROP_WINDOWS_ENDIAN_H

#include <stdint.h>

static inline uint16_t omdrop_htobe16(uint16_t value) {
  return (uint16_t)((value << 8) | (value >> 8));
}
static inline uint32_t omdrop_htobe32(uint32_t value) {
  return ((value & UINT32_C(0x000000ff)) << 24) |
         ((value & UINT32_C(0x0000ff00)) << 8) |
         ((value & UINT32_C(0x00ff0000)) >> 8) |
         ((value & UINT32_C(0xff000000)) >> 24);
}
static inline uint64_t omdrop_htobe64(uint64_t value) {
  return ((uint64_t)omdrop_htobe32((uint32_t)value) << 32) |
         omdrop_htobe32((uint32_t)(value >> 32));
}

#define htobe16 omdrop_htobe16
#define be16toh omdrop_htobe16
#define htole16(value) ((uint16_t)(value))
#define le16toh(value) ((uint16_t)(value))
#define htobe32 omdrop_htobe32
#define be32toh omdrop_htobe32
#define htole32(value) ((uint32_t)(value))
#define le32toh(value) ((uint32_t)(value))
#define htobe64 omdrop_htobe64
#define be64toh omdrop_htobe64
#define htole64(value) ((uint64_t)(value))
#define le64toh(value) ((uint64_t)(value))

#endif
