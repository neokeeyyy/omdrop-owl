# Windows backend lifecycle

`omdrop-windows-backend` is the current host-side lifecycle boundary. It
implements the documented command and status contract without changing the
Linux helper.

For deterministic tests, set:

```powershell
$env:OMDROP_WINDOWS_SIMULATED = "1"
$env:OMDROP_WINDOWS_STATE = "$env:TEMP\omdrop-windows.state"
```

In simulation mode, `start` performs a real protocol `HELLO` handshake against
the fake radio before persisting `visible=true`. With hardware, set
`OMDROP_RADIO_PORT` to a COM port; `start` opens it, sends `HELLO`, waits for
capabilities, and returns exit code 4 if the handshake fails.

On Windows, `start` launches a resident worker process after the handshake.
The worker owns the radio handle, keeps pumping the session, observes the
expiry timer and exits on `stop`. The persisted state includes the worker
process identifier for diagnostics. The current worker owns the radio
transport and lifecycle; the OWL/Ethernet runtime is the next integration
boundary and remains independently testable through its existing adapters.

## Build and test

Configure the complete Windows host tree from the repository root:

```powershell
cmake -S windows -B build/windows -G Ninja -DBUILD_TESTING=ON
cmake --build build/windows
ctest --test-dir build/windows --output-on-failure
```

`windows/firmware` is an independent ESP-IDF project and is not included by
the host CMake build. The root Windows build composes protocol, transport,
fake-radio, Ethernet, OWL, and backend targets without compiling their source
files more than once.
