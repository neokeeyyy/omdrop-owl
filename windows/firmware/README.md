# ESP32-S3 radio firmware

This is the ESP-IDF firmware endpoint for the Windows radio backend. It shares
the `OWLR` protocol implementation from `windows/protocol/` and uses UART 0 at
115200 baud with 8 data bits, no parity and one stop bit.

The initial QEMU target validates framing, CRC, sequence handling, handshake,
channel/state commands and transmit-result handling. QEMU does not emulate the
ESP32-S3 Wi-Fi radio, so this firmware reports no promiscuous-RX or raw-TX
capabilities and rejects transmitted frames.

## Build and run

Activate ESP-IDF 5.3.1 with the Python 3.11 environment, then run:

```powershell
idf.py set-target esp32s3
idf.py build
idf.py qemu monitor
```

For the automated protocol test, build the firmware and run:

```powershell
idf.py build
python qemu_protocol_test.py
```

The test creates the QEMU flash image when necessary and covers `HELLO`,
`CAPABILITIES`, channel selection, RX state, `STATUS`, rejected `TX_FRAME`,
`SET_MAC`, stop-RX, a malformed CRC frame and recovery of the next sequence.

The firmware can later be flashed to an ESP32-S3 without changing the OWLR
protocol endpoint. Real Wi-Fi RX/TX validation still requires hardware.
