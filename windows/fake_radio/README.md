# Fake radio

The fake radio implements the host/radio message exchange in memory. It is
used by tests and does not access a COM port, Wi-Fi adapter, ESP32, or Apple
device.

It responds to `HELLO`, `SET_CHANNEL`, `START_RX`, `STOP_RX`, `SET_MAC`, and
`TX_FRAME`. Tests can call `omdrop_fake_radio_inject_rx` to place an
`RX_FRAME` response in the radio-to-host queue.

Build and test it with:

```powershell
cmake -S windows -B build/windows -G Ninja -DCMAKE_C_COMPILER=gcc
cmake --build build/windows --target fake_radio_test
ctest --test-dir build/windows -R fake_radio_test --output-on-failure
```
