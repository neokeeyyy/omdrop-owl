# External radio protocol

This directory contains the small, platform-neutral wire format used between
the Windows OWL backend and an external radio such as an ESP32.

The protocol is deliberately independent of USB, serial APIs, C#, and OWL.
Those layers are responsible for transporting complete protocol frames. A
transport must preserve frame order and report disconnects; it must not
silently truncate or rewrite frames.

## Frame format

Every frame has a 20-byte little-endian header followed by its payload:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 4 | ASCII magic `OWLR` |
| 4 | 1 | Protocol version |
| 5 | 1 | Message type |
| 6 | 2 | Flags |
| 8 | 4 | Sequence number |
| 12 | 4 | Payload length |
| 16 | 4 | CRC-32 of the payload |
| 20 | N | Payload |

The current protocol version is `1` and payloads are limited to 4096 bytes.
The sequence number is assigned by the sender and allows a transport to detect
loss or reordering. After the first frame, each sequence must equal the
previous sequence plus one modulo `2^32`; a gap or duplicate is a transport
error. The CRC is the standard reflected CRC-32 used by Ethernet
(polynomial `0xedb88320`, initial value and final XOR `0xffffffff`).

Unknown message types must be rejected until capability negotiation defines
their semantics. A receiver must reject invalid magic, versions, lengths, and
CRCs without exposing the payload to higher layers.

## Message types

| Value | Name | Direction |
| ---: | --- | --- |
| 1 | `HELLO` | host → radio |
| 2 | `CAPABILITIES` | radio → host |
| 3 | `SET_CHANNEL` | host → radio |
| 4 | `START_RX` | host → radio |
| 5 | `STOP_RX` | host → radio |
| 6 | `RX_FRAME` | radio → host |
| 7 | `TX_FRAME` | host → radio |
| 8 | `TX_RESULT` | radio → host |
| 9 | `SET_MAC` | host → radio |
| 10 | `STATUS` | radio → host |

All message payloads use little-endian integers. The host and radio must
reject a payload whose size or reserved fields do not match its schema.

### `HELLO` (host to radio)

Six bytes: `host_features` (`u32`) and `max_frame_size` (`u16`).

### `CAPABILITIES` (radio to host)

Eighteen bytes: hardware major/minor (`u8`, `u8`), maximum frame size (`u16`),
feature bits (`u32`), supported-channel bitmask (`u32`), and radio MAC (`6
bytes`). Feature bits are `1` for promiscuous RX, `2` for raw TX, and `4` for
802.11 ACK support. The channel bitmask uses bit `N - 1` for channel `N`
within the first 32 channels.

### `SET_CHANNEL` (host to radio)

Four bytes: channel (`u16`), bandwidth (`u8`, MHz code), and flags (`u8`).
Channel zero is invalid. The bandwidth code is transport-defined until OWL
selects the required values; zero means the radio default.

### `START_RX` and `STOP_RX`

These messages have an empty payload.

### `RX_FRAME` and `TX_FRAME`

Both use an eight-byte metadata prefix followed by the 802.11 frame:
channel (`u16`), RSSI (`i8`, RX only), rate (`u8`), frame length (`u16`), and
reserved (`u16`, must be zero). `TX_FRAME` sets RSSI to zero. The maximum
802.11 frame is `OMDROP_RADIO_MAX_PAYLOAD - 8` bytes.

### `TX_RESULT` (radio to host)

Four bytes: status (`u8`), retries (`u8`), and two reserved zero bytes.
Statuses are `0` successful, `1` rejected, `2` busy, and `3` hardware error.

### `SET_MAC` (host to radio)

Six bytes containing the radio MAC address.

### `STATUS` (radio to host)

Twelve bytes: state (`u8`), channel (`u16`), error (`u8`), received frame
count (`u32`), and transmitted frame count (`u32`). States are `0` idle,
`1` receiving, and `2` error.

The C helpers in `radio_messages.h` and `radio_messages.c` encode and validate
these schemas without relying on compiler struct packing.

## Building the protocol tests

From a CMake-capable environment:

```sh
cmake -S windows/protocol -B build/windows-protocol
cmake --build build/windows-protocol
ctest --test-dir build/windows-protocol --output-on-failure
```
