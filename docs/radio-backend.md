# Radio backend contract

`omdrop` talks to a radio backend through one discoverability helper. The
Linux implementation is `userspace/omdrop-discoverable`; other platforms must
provide the same commands and observable behavior without depending on the
Linux implementation.

The contract is currently `0.8.1`. A backend reports its implementation
version separately from the contract version in `probe --json`.

## Commands

```text
omdrop-discoverable start <seconds>
omdrop-discoverable stop
omdrop-discoverable status [--json]
omdrop-discoverable peers
omdrop-discoverable probe [--json]
```

`start 0` keeps the computer discoverable until `stop`. A positive value is a
discoverability window in seconds. Calling `start` while already discoverable
extends the window and returns exit code 5.

The command name is descriptive rather than prescriptive. A Windows backend
may use a native executable or service, but its adapter for `omdrop` must
preserve these operations.

## Exit codes

| Code | Meaning |
| ---: | --- |
| 0 | The requested operation succeeded. |
| 2 | Invalid usage or the operation requires the required privilege. |
| 3 | The radio cannot become discoverable now, but retrying may succeed. |
| 4 | No usable AWDL radio is available, or the AWDL interface did not start. |
| 5 | Already discoverable; the existing window was extended. |

`status`, `peers`, and `probe` return 0 for a valid query, including when the
radio is currently unavailable. They describe the problem in their output.

Errors must be written to stderr and must not be reported as a successful
state. A backend should include a useful, human-readable explanation.

## `status`

Human-readable output is intended for diagnostics. `status --json` emits one
JSON object:

```json
{
  "visible": true,
  "remaining": 594,
  "reason": "",
  "rx_proven": true,
  "peer_op": ""
}
```

Fields:

- `visible` (`boolean`): whether the computer is currently discoverable.
- `remaining` (`integer|null`): seconds left in a timed window, or `null` for
  an untimed window or when not visible.
- `reason` (`string`): the latest stop or start failure reason, if known.
- `rx_proven` (`boolean`): whether a peer has been observed during the current
  window. Transmission alone does not set this field.
- `peer_op` (`string`): reserved for a future peer operation; currently empty.

The value of `remaining` is allowed to decrease between calls. Consumers must
not require exact second-level stability.

## `peers`

Each active peer is printed on one line:

```text
86:08:04:8a:2b:f4 rssi=?
```

The address is a lowercase, six-octet MAC address. `rssi` is an integer in dBm
when the backend can provide it; `?` is valid when signal strength is
unavailable. An empty result means that no peer is currently known.

Addresses are ephemeral: AWDL peers may randomize their address, so callers
must not treat the address as a stable device identity.

## `probe`

The human-readable form prints `id|message` rows, one per missing condition,
or `ready` when no condition is blocking the backend.

`probe --json` emits:

```json
{
  "backend": "owl",
  "version": "0.1.0",
  "contract": "0.8.1",
  "hardware": true,
  "missing": []
}
```

Fields:

- `backend` (`string`): stable backend identifier.
- `version` (`string`): implementation version.
- `contract` (`string`): supported contract version.
- `hardware` (`boolean`): whether a suitable radio device is present.
- `missing` (`array`): blocking conditions, each with an `id` and user-facing
  `say` string.

`missing` should report the most fundamental blocking conditions first. Known
condition IDs include `hardware`, `backend`, and `radio`; consumers should
display unknown IDs rather than rejecting them.

## Privileges and lifecycle

`status`, `peers`, and `probe` must be usable without elevated privileges.
`start` and `stop` may require privileges to configure the radio. The backend
must clean up its AWDL interface and child processes on `stop`, including after
an expired timed window.

If the radio process exits unexpectedly, `status` must eventually report
`visible: false` and `start` must be able to recover without requiring a
machine reboot.

## Platform mapping

The Linux backend uses monitor/IBSS interfaces, NetworkManager, polkit, and
OWL. A Windows backend is expected to replace those platform-specific pieces
with its own service, permissions model, and radio transport (for example an
external radio), while preserving the command and output contract above.

The AirDrop protocol and the radio lifecycle are separate concerns. A Windows
port should not copy Linux-specific commands such as `iw`, `ip`, `nmcli`, or
`pkexec` into a compatibility layer.
