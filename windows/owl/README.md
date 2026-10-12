# OWL Windows port

This target currently builds the platform-neutral AWDL core from the pinned
OWL checkout. It deliberately does not compile OWL's Linux daemon yet.

On Windows/MinGW, the target supplies small compatibility headers for the
POSIX networking types and byte-order helpers used by the shared core. These
headers are confined to this target; the pinned OWL checkout is not modified.
The core smoke test currently passes with the UCRT64 GCC toolchain.

The upstream daemon has three platform-specific boundaries that must be
replaced for Windows:

- `daemon/io.c`: pcap, monitor/TUN interfaces, and Linux/macOS network APIs;
- `daemon/netutils.c`: netlink and interface/neighbour configuration;
- `daemon/owl.c` and `daemon/core.c`: POSIX process APIs and libev wiring.

The Windows implementation will connect those boundaries to
`windows/transport/` and a Windows virtual adapter. Keeping the core as a
separate target lets us verify that AWDL parsing, scheduling, peer handling,
and frame construction remain shared code.

`radio_link` is the first Windows-side boundary: it maps typed radio protocol
messages to the serial transport without depending on pcap, libev, or a
particular serial device. Its tests exercise framing, sequence handling, and
HELLO/TX message serialization. It is intentionally not the OWL daemon yet;
`radio_adapter` now translates OWL's raw radiotap frames to `TX_FRAME` and
passes received `RX_FRAME` messages synchronously through `awdl_rx`. Ethernet
delivery is exposed as a callback so the future Windows virtual adapter can be
added without changing OWL's shared core. `radio_session` now provides the
small synchronous lifecycle around that link: HELLO, capabilities, channel,
RX start/stop, status, TX results, and an injectable message callback for
received frames. A future event loop can call its pump method from a COM
reader without changing these contracts. `omdrop_radio_serial_session` is the
concrete COM wrapper: it owns the Win32 serial handle and translates its
read/write operations into the session callbacks. A zero-length read is
treated as a timeout and is not considered a protocol error; a failed read or
write is surfaced to the caller. `radio_loop` provides the first event-loop
boundary as a small pump runner with an explicit stop predicate and an
optional iteration bound for tests; it does not create a hidden thread or
change COM ownership. `radio_runtime` now composes the session and adapter:
RX messages automatically reach `awdl_rx`, while raw OWL frames can be sent
through the same session and their `TX_RESULT` remains observable. The
runtime still requires its caller to provide the OWL state and owns no
network interface.

The `radio_runtime_test` is the virtual end-to-end check for this layer. It
uses the fake radio, initializes independent OWL sender/receiver states,
generates a real PSF radiotap frame with OWL, sends a raw TX frame, checks its
TX result, injects the generated PSF through the RX protocol path, and verifies
that the receiving OWL state processes it. This is the complete virtual radio
pipeline; it does not claim AirDrop compatibility, a virtual Ethernet device,
or physical ESP32/iPhone validation.

## Build

From the repository root:

```powershell
pwsh .\tools\windows\fetch-owl.ps1
cmake -S windows/owl -B build/windows-owl -G Ninja -DCMAKE_C_COMPILER=gcc
cmake --build build/windows-owl
ctest --test-dir build/windows-owl --output-on-failure
```
