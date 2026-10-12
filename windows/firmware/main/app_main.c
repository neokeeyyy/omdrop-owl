#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "radio_messages.h"
#include "radio_protocol.h"

#include <string.h>

#define RADIO_UART UART_NUM_0
#define RADIO_BAUD_RATE 115200
#define RADIO_RX_BUFFER_SIZE 8192
#define RADIO_FRAME_BUFFER_SIZE \
  (OMDROP_RADIO_HEADER_SIZE + OMDROP_RADIO_MAX_PAYLOAD)

static const char *TAG = "omdrop-radio";

struct radio_state {
  uint8_t state;
  uint16_t channel;
  uint8_t error;
  uint32_t rx_frames;
  uint32_t tx_frames;
  uint32_t next_tx_sequence;
  uint32_t expected_rx_sequence;
  bool have_rx_sequence;
  uint8_t mac[OMDROP_RADIO_MAC_SIZE];
};

static struct radio_state radio = {
    .state = OMDROP_RADIO_STATE_IDLE,
    .channel = 1,
    .next_tx_sequence = 1,
    .mac = {0x02, 0x4f, 0x4d, 0x44, 0x52, 0x01},
};

static int send_message(uint8_t type, const uint8_t *payload,
                        size_t payload_length) {
  uint8_t frame[RADIO_FRAME_BUFFER_SIZE];
  struct omdrop_radio_message message = {
      .version = OMDROP_RADIO_PROTOCOL_VERSION,
      .type = type,
      .flags = 0,
      .sequence = radio.next_tx_sequence++,
      .payload = payload,
      .payload_length = payload_length,
  };
  size_t frame_length;

  if (!omdrop_radio_encode(&message, frame, sizeof(frame), &frame_length)) {
    return 0;
  }
  return uart_write_bytes(RADIO_UART, frame, frame_length) == (int)frame_length;
}

static void send_capabilities(void) {
  struct omdrop_radio_capabilities capabilities = {
      .hardware_major = 1,
      .hardware_minor = 0,
      .max_frame_size = OMDROP_RADIO_MAX_FRAME_SIZE,
      .features = 0,
      .channel_mask = UINT32_C(0x00007fff),
  };
  uint8_t payload[18];
  size_t payload_length;

  memcpy(capabilities.mac, radio.mac, sizeof(radio.mac));
  if (omdrop_radio_encode_capabilities(&capabilities, payload, sizeof(payload),
                                       &payload_length)) {
    send_message(OMDROP_RADIO_CAPABILITIES, payload, payload_length);
  }
}

static void send_status(void) {
  struct omdrop_radio_status status = {
      .state = radio.state,
      .channel = radio.channel,
      .error = radio.error,
      .rx_frames = radio.rx_frames,
      .tx_frames = radio.tx_frames,
  };
  uint8_t payload[12];
  size_t payload_length;

  if (omdrop_radio_encode_status(&status, payload, sizeof(payload),
                                 &payload_length)) {
    send_message(OMDROP_RADIO_STATUS, payload, payload_length);
  }
}

static void send_tx_result(uint8_t result) {
  struct omdrop_radio_tx_result tx_result = {
      .status = result,
      .retries = 0,
  };
  uint8_t payload[4];
  size_t payload_length;

  if (omdrop_radio_encode_tx_result(&tx_result, payload, sizeof(payload),
                                    &payload_length)) {
    send_message(OMDROP_RADIO_TX_RESULT, payload, payload_length);
  }
}

static void handle_message(const struct omdrop_radio_message *message) {
  struct omdrop_radio_channel channel;
  struct omdrop_radio_hello hello;
  uint8_t mac[OMDROP_RADIO_MAC_SIZE];

  if (radio.have_rx_sequence &&
      !omdrop_radio_sequence_is_next(radio.expected_rx_sequence,
                                     message->sequence)) {
    radio.error = 1;
    send_status();
    return;
  }
  radio.expected_rx_sequence = message->sequence;
  radio.have_rx_sequence = true;

  if (!(message->type == OMDROP_RADIO_STATUS &&
        message->payload_length == 0) &&
      !omdrop_radio_validate_payload(message->type, message->payload,
                                     message->payload_length)) {
    radio.error = 2;
    send_status();
    return;
  }

  switch (message->type) {
    case OMDROP_RADIO_HELLO:
      if (omdrop_radio_decode_hello(message->payload, message->payload_length,
                                    &hello)) {
        send_capabilities();
      }
      break;
    case OMDROP_RADIO_SET_CHANNEL:
      if (omdrop_radio_decode_channel(message->payload, message->payload_length,
                                      &channel)) {
        radio.channel = channel.channel;
        radio.error = 0;
      } else {
        radio.error = 3;
      }
      send_status();
      break;
    case OMDROP_RADIO_START_RX:
      radio.state = OMDROP_RADIO_STATE_RX;
      radio.error = 0;
      send_status();
      break;
    case OMDROP_RADIO_STOP_RX:
      radio.state = OMDROP_RADIO_STATE_IDLE;
      radio.error = 0;
      send_status();
      break;
    case OMDROP_RADIO_TX_FRAME:
      radio.tx_frames++;
      send_tx_result(OMDROP_RADIO_TX_REJECTED);
      break;
    case OMDROP_RADIO_SET_MAC:
      if (omdrop_radio_decode_set_mac(message->payload, message->payload_length,
                                      mac)) {
        memcpy(radio.mac, mac, sizeof(radio.mac));
        radio.error = 0;
      } else {
        radio.error = 4;
      }
      send_status();
      break;
    case OMDROP_RADIO_STATUS:
      send_status();
      break;
    default:
      radio.error = 5;
      send_status();
      break;
  }
}

static void process_uart(void) {
  static uint8_t input[RADIO_FRAME_BUFFER_SIZE];
  static size_t input_length;
  uint8_t chunk[256];
  int received = uart_read_bytes(RADIO_UART, chunk, sizeof(chunk),
                                 pdMS_TO_TICKS(20));

  if (received <= 0) {
    return;
  }
  if (input_length + (size_t)received > sizeof(input)) {
    input_length = 0;
    radio.error = 5;
    return;
  }
  memcpy(input + input_length, chunk, (size_t)received);
  input_length += (size_t)received;

  while (input_length >= OMDROP_RADIO_HEADER_SIZE) {
    struct omdrop_radio_message message;
    size_t frame_length;

    if (input[0] != OMDROP_RADIO_MAGIC_0 ||
        input[1] != OMDROP_RADIO_MAGIC_1 ||
        input[2] != OMDROP_RADIO_MAGIC_2 ||
        input[3] != OMDROP_RADIO_MAGIC_3) {
      memmove(input, input + 1, --input_length);
      continue;
    }
    if (!omdrop_radio_decode(input, input_length, &message, &frame_length)) {
      uint32_t payload_length = (uint32_t)input[12] |
                                ((uint32_t)input[13] << 8) |
                                ((uint32_t)input[14] << 16) |
                                ((uint32_t)input[15] << 24);
      if (payload_length > OMDROP_RADIO_MAX_PAYLOAD ||
          input_length >= OMDROP_RADIO_HEADER_SIZE + payload_length) {
        memmove(input, input + 1, --input_length);
      }
      break;
    }
    handle_message(&message);
    input_length -= frame_length;
    memmove(input, input + frame_length, input_length);
  }
}

void app_main(void) {
  const uart_config_t config = {
      .baud_rate = RADIO_BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  ESP_ERROR_CHECK(uart_driver_install(RADIO_UART, RADIO_RX_BUFFER_SIZE, 0, 0,
                                      NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(RADIO_UART, &config));
  ESP_LOGI(TAG, "OWLR firmware ready");

  while (true) {
    process_uart();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
