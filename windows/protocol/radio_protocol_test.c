#include "radio_protocol.h"
#include "radio_messages.h"

#include <assert.h>
#include <string.h>

int main(void) {
  static const uint8_t payload[] = {0x00, 0x01, 0xfe, 0xff};
  uint8_t encoded[OMDROP_RADIO_HEADER_SIZE + sizeof(payload)];
  struct omdrop_radio_message input = {
      OMDROP_RADIO_PROTOCOL_VERSION,
      OMDROP_RADIO_TX_FRAME,
      0x1234,
      42,
      payload,
      sizeof(payload),
  };
  struct omdrop_radio_message output;
  size_t encoded_length;
  size_t frame_length;

  assert(omdrop_radio_encode(&input, encoded, sizeof(encoded),
                             &encoded_length));
  assert(encoded_length == sizeof(encoded));
  assert(omdrop_radio_decode(encoded, encoded_length, &output, &frame_length));
  assert(frame_length == encoded_length);
  assert(output.version == input.version);
  assert(output.type == input.type);
  assert(output.flags == input.flags);
  assert(output.sequence == input.sequence);
  assert(output.payload_length == input.payload_length);
  assert(memcmp(output.payload, payload, sizeof(payload)) == 0);

  encoded[OMDROP_RADIO_HEADER_SIZE] ^= 1;
  assert(!omdrop_radio_decode(encoded, encoded_length, &output, &frame_length));
  assert(omdrop_radio_sequence_is_next(41, 42));
  assert(omdrop_radio_sequence_is_next(UINT32_MAX, 0));
  assert(!omdrop_radio_sequence_is_next(41, 43));

  {
    uint8_t buffer[OMDROP_RADIO_MAX_PAYLOAD];
    size_t length;
    struct omdrop_radio_hello hello = {OMDROP_RADIO_FEATURE_RAW_TX, 1200};
    struct omdrop_radio_hello decoded_hello;
    struct omdrop_radio_frame tx = {6, 0, 0, sizeof(payload), payload};
    struct omdrop_radio_frame decoded_tx;
    struct omdrop_radio_status status = {OMDROP_RADIO_STATE_RX, 6, 0, 12, 4};
    struct omdrop_radio_status decoded_status;
    uint8_t mac[OMDROP_RADIO_MAC_SIZE] = {0, 1, 2, 3, 4, 5};
    uint8_t decoded_mac[OMDROP_RADIO_MAC_SIZE];

    assert(omdrop_radio_encode_hello(&hello, buffer, sizeof(buffer), &length));
    assert(omdrop_radio_validate_payload(OMDROP_RADIO_HELLO, buffer, length));
    assert(omdrop_radio_decode_hello(buffer, length, &decoded_hello));
    assert(decoded_hello.host_features == hello.host_features);
    assert(decoded_hello.max_frame_size == hello.max_frame_size);

    assert(omdrop_radio_encode_frame(&tx, buffer, sizeof(buffer), &length));
    assert(omdrop_radio_validate_payload(OMDROP_RADIO_TX_FRAME, buffer, length));
    assert(omdrop_radio_decode_frame(buffer, length, &decoded_tx));
    assert(decoded_tx.channel == tx.channel);
    assert(decoded_tx.frame_length == tx.frame_length);
    assert(memcmp(decoded_tx.frame, payload, sizeof(payload)) == 0);

    assert(omdrop_radio_encode_status(&status, buffer, sizeof(buffer), &length));
    assert(omdrop_radio_decode_status(buffer, length, &decoded_status));
    assert(decoded_status.rx_frames == status.rx_frames);
    assert(!omdrop_radio_validate_payload(OMDROP_RADIO_STATUS, buffer, 11));

    assert(omdrop_radio_encode_set_mac(mac, buffer, sizeof(buffer), &length));
    assert(omdrop_radio_decode_set_mac(buffer, length, decoded_mac));
    assert(memcmp(mac, decoded_mac, sizeof(mac)) == 0);
    assert(omdrop_radio_validate_payload(OMDROP_RADIO_START_RX, NULL, 0));
    assert(!omdrop_radio_validate_payload(0xff, NULL, 0));
  }

  return 0;
}
