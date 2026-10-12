#include "serial_transport.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

struct omdrop_radio_serial {
  HANDLE handle;
  uint32_t timeout_ms;
};
#endif

static void discard_prefix(struct omdrop_radio_parser *parser, size_t count) {
  if (count >= parser->length) {
    parser->length = 0;
    return;
  }
  memmove(parser->buffer, parser->buffer + count, parser->length - count);
  parser->length -= count;
}

void omdrop_radio_parser_init(struct omdrop_radio_parser *parser) {
  if (parser == NULL) return;
  memset(parser, 0, sizeof(*parser));
}

int omdrop_radio_parser_feed(struct omdrop_radio_parser *parser,
                             const uint8_t *data, size_t length,
                             struct omdrop_radio_message *message) {
  size_t frame_length;
  size_t decoded_length;
  size_t offset;

  if (parser == NULL || message == NULL ||
      (length != 0 && data == NULL) ||
      length > sizeof(parser->buffer) - parser->length) {
    return OMDROP_RADIO_TRANSPORT_ERROR;
  }
  if (length != 0) {
    memcpy(parser->buffer + parser->length, data, length);
    parser->length += length;
  }

  for (;;) {
    if (parser->length < OMDROP_RADIO_HEADER_SIZE) {
      return OMDROP_RADIO_TRANSPORT_NEED_MORE;
    }

    if (parser->buffer[0] != OMDROP_RADIO_MAGIC_0 ||
        parser->buffer[1] != OMDROP_RADIO_MAGIC_1 ||
        parser->buffer[2] != OMDROP_RADIO_MAGIC_2 ||
        parser->buffer[3] != OMDROP_RADIO_MAGIC_3) {
      for (offset = 1; offset + 3 < parser->length; ++offset) {
        if (parser->buffer[offset] == OMDROP_RADIO_MAGIC_0 &&
            parser->buffer[offset + 1] == OMDROP_RADIO_MAGIC_1 &&
            parser->buffer[offset + 2] == OMDROP_RADIO_MAGIC_2 &&
            parser->buffer[offset + 3] == OMDROP_RADIO_MAGIC_3) {
          break;
        }
      }
      discard_prefix(parser, offset);
      if (parser->length < OMDROP_RADIO_HEADER_SIZE) {
        return OMDROP_RADIO_TRANSPORT_NEED_MORE;
      }
    }

    if (!omdrop_radio_decode(parser->buffer, parser->length, message,
                             &frame_length)) {
      uint32_t payload_length =
          (uint32_t)parser->buffer[12] |
          ((uint32_t)parser->buffer[13] << 8) |
          ((uint32_t)parser->buffer[14] << 16) |
          ((uint32_t)parser->buffer[15] << 24);
      if (parser->buffer[4] != OMDROP_RADIO_PROTOCOL_VERSION ||
          parser->buffer[5] == 0 ||
          parser->buffer[5] > OMDROP_RADIO_STATUS ||
          payload_length > OMDROP_RADIO_MAX_PAYLOAD) {
        discard_prefix(parser, 1);
        continue;
      }
      if (parser->length >= OMDROP_RADIO_HEADER_SIZE + payload_length) {
        discard_prefix(parser, 1);
        continue;
      }
      return OMDROP_RADIO_TRANSPORT_NEED_MORE;
    }

    if (parser->has_expected_sequence &&
        !omdrop_radio_sequence_is_next(parser->expected_sequence,
                                       message->sequence)) {
      parser->expected_sequence = message->sequence;
      memcpy(parser->frame, parser->buffer, frame_length);
      discard_prefix(parser, frame_length);
      if (!omdrop_radio_decode(parser->frame, frame_length, message,
                               &decoded_length)) {
        return OMDROP_RADIO_TRANSPORT_ERROR;
      }
      return OMDROP_RADIO_TRANSPORT_SEQUENCE_GAP;
    }

    parser->expected_sequence = message->sequence;
    parser->has_expected_sequence = 1;
    memcpy(parser->frame, parser->buffer, frame_length);
    discard_prefix(parser, frame_length);
    if (!omdrop_radio_decode(parser->frame, frame_length, message,
                             &decoded_length)) {
      return OMDROP_RADIO_TRANSPORT_ERROR;
    }
    return OMDROP_RADIO_TRANSPORT_OK;
  }
}

#ifdef _WIN32
static int configure_serial(HANDLE handle, uint32_t baud_rate,
                            uint32_t timeout_ms) {
  DCB state;
  COMMTIMEOUTS timeouts;

  memset(&state, 0, sizeof(state));
  state.DCBlength = sizeof(state);
  if (!GetCommState(handle, &state)) return 0;
  state.BaudRate = baud_rate;
  state.ByteSize = 8;
  state.Parity = NOPARITY;
  state.StopBits = ONESTOPBIT;
  state.fBinary = TRUE;
  state.fDtrControl = DTR_CONTROL_DISABLE;
  state.fRtsControl = RTS_CONTROL_DISABLE;
  if (!SetCommState(handle, &state)) return 0;

  memset(&timeouts, 0, sizeof(timeouts));
  timeouts.ReadIntervalTimeout = timeout_ms;
  timeouts.ReadTotalTimeoutConstant = timeout_ms;
  timeouts.WriteTotalTimeoutConstant = timeout_ms;
  return SetCommTimeouts(handle, &timeouts) != 0;
}

struct omdrop_radio_serial *omdrop_radio_serial_open(const char *port_name,
                                                      uint32_t baud_rate,
                                                      uint32_t timeout_ms) {
  struct omdrop_radio_serial *serial;
  HANDLE handle;
  char device_name[64];

  if (port_name == NULL || port_name[0] == '\0') return NULL;
  if (strncmp(port_name, "\\\\.\\", 4) == 0) {
    if (strlen(port_name) >= sizeof(device_name)) return NULL;
    strcpy(device_name, port_name);
  } else {
    if (snprintf(device_name, sizeof(device_name), "\\\\.\\%s", port_name) < 0)
      return NULL;
  }
  handle = CreateFileA(device_name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, 0, NULL);
  if (handle == INVALID_HANDLE_VALUE) return NULL;
  if (!configure_serial(handle, baud_rate, timeout_ms)) {
    CloseHandle(handle);
    return NULL;
  }
  serial = (struct omdrop_radio_serial *)HeapAlloc(
      GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*serial));
  if (serial == NULL) {
    CloseHandle(handle);
    return NULL;
  }
  serial->handle = handle;
  serial->timeout_ms = timeout_ms;
  return serial;
}

int omdrop_radio_serial_read(struct omdrop_radio_serial *serial, uint8_t *data,
                             size_t capacity, size_t *read_length) {
  DWORD count;
  if (serial == NULL || data == NULL || read_length == NULL ||
      capacity == 0 || capacity > (size_t)MAXDWORD)
    return 0;
  if (!ReadFile(serial->handle, data, (DWORD)capacity, &count, NULL)) return 0;
  *read_length = count;
  return 1;
}

int omdrop_radio_serial_write(struct omdrop_radio_serial *serial,
                              const uint8_t *data, size_t length) {
  size_t offset = 0;
  while (offset < length) {
    DWORD count;
    size_t remaining = length - offset;
    DWORD request = remaining > (size_t)MAXDWORD ? MAXDWORD : (DWORD)remaining;
    if (!WriteFile(serial->handle, data + offset, request, &count, NULL) ||
        count == 0)
      return 0;
    offset += count;
  }
  return 1;
}

void omdrop_radio_serial_close(struct omdrop_radio_serial *serial) {
  if (serial == NULL) return;
  CloseHandle(serial->handle);
  HeapFree(GetProcessHeap(), 0, serial);
}
#else
struct omdrop_radio_serial *omdrop_radio_serial_open(const char *port_name,
                                                      uint32_t baud_rate,
                                                      uint32_t timeout_ms) {
  (void)port_name;
  (void)baud_rate;
  (void)timeout_ms;
  return NULL;
}

int omdrop_radio_serial_read(struct omdrop_radio_serial *serial, uint8_t *data,
                             size_t capacity, size_t *read_length) {
  (void)serial;
  (void)data;
  (void)capacity;
  (void)read_length;
  return 0;
}

int omdrop_radio_serial_write(struct omdrop_radio_serial *serial,
                              const uint8_t *data, size_t length) {
  (void)serial;
  (void)data;
  (void)length;
  return 0;
}

void omdrop_radio_serial_close(struct omdrop_radio_serial *serial) {
  (void)serial;
}
#endif
