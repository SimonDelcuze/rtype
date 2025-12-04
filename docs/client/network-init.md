# Network Init Handshake

Client side startup sends a short handshake and keeps retrying until the server replies.

## Messages sent

- `ClientHello` (0x01)
- `ClientJoinRequest` (0x02)
- `ClientReady` (0x03)
- `ClientPing` (0x04)

These four are sent once receiver/sender threads are started. A “welcome loop” repeats them every second until a server packet (LevelInit or Snapshot) is received.

## Stop condition

- `NetworkMessageHandler` sets a shared `handshakeDone` flag when it successfully parses a `LevelInit` or `Snapshot`.
- The welcome loop exits when `handshakeDone == true`.

## Server expectations

- Listen on the configured port (mock: 50010) and reply with valid server packets (LevelInit or Snapshot) so the client stops retrying.
- Optionally echo pings as server-to-client `ClientPing` (0x04) with server packet type.

## Ports (mock setup)

- Client binds receiver on 50000, sends to 127.0.0.1:50010.
- Mock server binds on 50010, sends to client 50000.
