# Windows Ethernet boundary

`omdrop-virtual-ethernet` defines the host-facing packet boundary without
committing the backend to a particular Windows driver. The memory
implementation is used by virtual tests:

- `host_send` queues an Ethernet packet for the backend to read;
- `read` consumes packets queued by the host;
- `write` queues packets produced by OWL;
- `host_receive` consumes packets delivered back to the host.

The queues enforce the Ethernet frame limit and report full, empty, and
invalid-buffer conditions explicitly. A Wintun implementation can replace
this backend while preserving the callback shape. This target does not create
a Windows network adapter yet.
