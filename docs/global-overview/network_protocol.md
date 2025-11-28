#RType UDP Protocol Documentation

## Protocol Overview

The RType UDP Protocol is a lightweight, binary network protocol designed for fast, real-time communication between clients and the game server. It is optimized for arcade-style gameplay running at high update frequencies (e.g., 60Hz), where low latency, efficient data transfer, and resistance to packet loss are essential.

All messages follow a compact binary header that includes a magic number, protocol version, packet type, message type, sequence ID, server tick ID, and payload length. This ensures that packets can be validated, ordered, and processed reliably even under unstable network conditions.

The protocol is strictly **binary** and avoids text-based encoding to minimize packet size and maximize bandwidth efficiency. Communication is performed exclusively over **UDP**, with optional acknowledgment messages for reliability when required by specific interactions.

A key feature of this protocol is the use of **delta-state snapshots**:  
the server does not send full world states every tick.  
Instead, for each entity, only the fields that have changed since the last acknowledged snapshot are transmitted. A 16-bit **Update Mask** indicates which fields are included in each update, allowing the server to send extremely compact packets and scale efficiently with a large number of entities.

This design keeps network usage low while maintaining accurate and responsive synchronization between the server’s authoritative simulation and the client’s predicted rendering.

## Header Structure

- **Magic Number** (4 bytes): Fixed value `0xA3 0x5F 0xC8 0x1D` to identify the protocol.
- **Version** (1 byte): Protocol version number.
- **Packet Type** (1 byte): Identifies the type of packet.
  - 0x01: Client to Server Packet
  - 0x02: Server to Client Packet
- **Message Type** (1 byte): Specific message identifier within the packet type. 
  - 0x01: CLIENT_HELLO
  - 0x02: CLIENT_JOIN_REQUEST
  - 0x03: CLIENT_READY
  - 0x04: CLIENT_PING
  - 0x05: CLIENT_INPUT
  - 0x06: CLIENT_ACKNOWLEDGE
  - 0x07: CLIENT_DISCONNECT
  - 0x10: SERVER_HELLO
  - 0x11: SERVER_JOIN_ACCEPT
  - 0x12: SERVER_JOIN_DENY
  - 0x13: SERVER_PONG
  - 0x14: SERVER_SNAPSHOT
  - 0x15: SERVER_GAME_START
  - 0x16: SERVER_GAME_END
  - 0x17: SERVER_KICK
  - 0x18: SERVER_BAN
  - 0x19: SERVER_BROADCAST
  - 0x1A: SERVER_DISCONNECT
  - 0x1B: SERVER_ACKNOWLEDGE
  - 0x1C: SERVER_PLAYER_DISCONNECTED
  - 0x1D: SERVER_ENTITY_SPAWN
  - 0x1E: SERVER_ENTITY_DESTROYED
- **Sequence Number** (2 bytes): Incremental number for packet ordering.
- **Tick ID** (4 bytes): Current server tick when the packet is sent.
- **Payload Length** (2 bytes): Length of the payload in bytes.


### CRC32 (Integrity Check)

All packets include an additional 4-byte CRC32 checksum appended at the end of the message.

- **CRC32** (4 bytes):  
  A checksum computed over the entire packet *excluding* the CRC32 field itself  
  (from Magic Number up to the end of the Payload).  
  Used to detect corrupted, truncated, or malformed packets.

**Final packet layout:**

```
[Header][Payload][CRC32]
```

### Verification

- The sender computes CRC32(Header + Payload) and appends it.
- The receiver recomputes CRC32 on the received data (excluding the last 4 bytes)  
  and compares the result with the transmitted CRC32.
- If they do not match, the packet is discarded.

CRC32 improves robustness in UDP environments by preventing clients or servers  
from processing corrupted snapshots or malformed data.


## Client to Server Packet Structure

### CLIENT_HELLO (0x01)
- Sent by the client to initiate a connection.
- Payload:
  - Client Version (1 byte)
  - Client Name Length (1 byte)
  - Client Name (variable length, UTF-8 or ASCII)

### CLIENT_JOIN_REQUEST (0x02)
- Sent by the client to request joining a game or lobby.
- Payload:
  - None

### CLIENT_READY (0x03)
- Sent by the client to indicate it is ready to start the match.
- Payload:
  - None

### CLIENT_PING (0x04)
- Sent by the client to measure round-trip latency.
- Payload:
  - Client Time (4 bytes)

### CLIENT_INPUT (0x05)
- Sent frequently (30–60Hz) to transmit player input for the current tick.
- Payload:
  - Player ID (4 bytes)
  - Input Flags (2 bytes)
  - X Position (4 bytes, float)
  - Y Position (4 bytes, float)
  - Angle (4 bytes, float)

### CLIENT_ACKNOWLEDGE (0x06)
- Sent to acknowledge the receipt of a server packet.
- Payload:
  - Acknowledged Sequence Number (2 bytes)
  - Acknowledged Message Type (1 byte)

### CLIENT_DISCONNECT (0x07)
- Sent by the client to disconnect cleanly from the server.
- Payload:
  - Reason Code (1 byte)

## Server to Client Packet Structure

### SERVER_HELLO (0x10)
- Sent by the server after receiving CLIENT_HELLO, confirming the connection.
- Payload:
  - Server Version (1 byte)
  - Assigned Player ID (4 bytes)

### SERVER_JOIN_ACCEPT (0x11)
- Sent by the server to confirm that the client is allowed to join the game or lobby.
- Payload:
  - Initial Game State ID (4 bytes)

### SERVER_JOIN_DENY (0x12)
- Sent by the server if the client is not allowed to join.
- Payload:
  - Reason Code (1 byte)

### SERVER_PONG (0x13)
- Sent by the server in response to CLIENT_PING.
- Payload:
  - Client Time (4 bytes)

### SERVER_SNAPSHOT (0x14)
- Sent at a regular interval (e.g., 60Hz) to deliver the current game state.
- Supports delta updates: only modified fields are included.
- Payload:
  - Entity Count (2 bytes)
  - For each entity:
    - Entity ID (4 bytes)
    - Update Mask (2 bytes)
    - If bit 0 set: Entity Type (1 byte)
    - If bit 1 set: X Position (4 bytes, float)
    - If bit 2 set: Y Position (4 bytes, float)
    - If bit 3 set: X Velocity (4 bytes, float)
    - If bit 4 set: Y Velocity (4 bytes, float)
    - If bit 5 set: Health (2 bytes)
    - If bit 6 set: Status Effects (1 byte)
    - If bit 7 set: Orientation (4 bytes, float)
    - If bit 8 set: Dead/Alive State (1 byte, 0 = alive, 1 = dead)
    - Additional fields can be added by extending the mask

### SERVER_GAME_START (0x15)
- Sent by the server to indicate that the match is starting.
- Payload:
  - None

### SERVER_GAME_END (0x16)
- Sent by the server to indicate that the match has ended.
- Payload:
  - Winning Player ID (4 bytes)

### SERVER_KICK (0x17)
- Sent by the server to kick the client from the session.
- Payload:
  - Reason Code (1 byte)

### SERVER_BAN (0x18)
- Sent by the server to permanently ban the client.
- Payload:
  - Reason Code (1 byte)

### SERVER_BROADCAST (0x19)
- Sent by the server to broadcast a text message to all clients.
- Payload:
  - Message Length (1 byte)
  - Message Content (variable length)

### SERVER_DISCONNECT (0x1A)
- Sent by the server to force a clean disconnection.
- Payload:
  - Reason Code (1 byte)

### SERVER_ACKNOWLEDGE (0x1B)
- Sent by the server to acknowledge receipt of a client packet.
- Payload:
  - Acknowledged Sequence Number (2 bytes)
  - Acknowledged Message Type (1 byte)

## Reason Codes

Several messages in the protocol (such as `CLIENT_DISCONNECT`, `SERVER_KICK`, `SERVER_BAN`, and `SERVER_DISCONNECT`) include a **Reason Code** field.  
This field specifies why a disconnection or administrative action occurred.

Reason Codes are 1 byte (0–255).  
The following values are reserved:

### Generic Reason Codes
- **0x00 — Unknown Reason**  
  Used when no specific cause is provided.

- **0x01 — Requested by Client**  
  The client intentionally closed the connection (user pressed "Quit", etc.).

### Kick Reasons (Server → Client)
- **0x10 — Inactivity**  
  Client was removed due to prolonged inactivity.

- **0x11 — Protocol Violation**  
  Client sent invalid or malformed packets.

- **0x12 — Overflow / Spam**  
  Client exceeded message rate limits or produced excessive traffic.

- **0x13 — Unauthorized Action**  
  Client attempted an invalid or forbidden behavior.

### Ban Reasons (Server → Client)
- **0x20 — Cheating Detected**  
  Client exhibited behavior flagged by anticheat.

- **0x21 — Manual Ban**  
  Administrator or server logic explicitly banned the client.

### Server Shutdown Reasons
- **0x30 — Server Shutdown**  
  The server is closing gracefully.

- **0x31 — Server Restart**  
  The server is restarting and disconnecting clients safely.

---

### Notes
- Additional custom reason codes (0x80–0xFF) may be defined by game logic if needed.
- Clients should treat unknown reason codes as non-fatal and display a generic message.
