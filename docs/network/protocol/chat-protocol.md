# Chat Protocol

The R-Type project includes a custom chat system for inter-player communication within lobby rooms. It uses a binary protocol over UDP.

## Packet Structure

The `ChatPacket` is defined in `shared/include/network/ChatPacket.hpp`. It is a fixed-size structure following the common `PacketHeader`.

| Field | Type | Size | Description |
| :--- | :--- | :--- | :--- |
| **Room ID** | `uint32` | 4 bytes | Unique identifier of the room (0 for global/lobby). |
| **Player ID** | `uint32` | 4 bytes | Unique identifier of the sender. |
| **Player Name** | `char[32]` | 32 bytes | Null-terminated string representing the sender's name. |
| **Message** | `char[121]` | 121 bytes | Null-terminated string containing the text (max 120 chars). |

## Communication Flow

The chat follows a classic Client-Server-Client broadcast pattern.

### 1. Client to Server (Send)
When a player sends a message, the client creates a `ChatPacket` and sends it to the `LobbyServer`.

- **Sequence**: The packet uses the next session sequence ID.
- **Verification**: The server validates that the player is indeed in the specified `Room ID`.

### 2. Server Processing (Relay)
The `LobbyServer` handles the packet in `handleChatPacket`:
- Decodes the packet.
- Injects the authoritative sender name from the session.
- Identifies all other clients currently connected to the same room.

### 3. Server to Client (Broadcast)
The server re-encodes the packet and sends it individually to every player in that room (including the sender for visual confirmation).

## Technical Implementation

### Client Side
- **LobbyConnection**: Manages the UDP socket and parses incoming chat messages into a thread-safe queue (`chatMessages_`).
- **LobbyMenu / RoomWaitingMenu**: Polls the queue and displays messages in the UI.

### Server Side
- **LobbyServer**: Receives `MessageType::Chat` packets.
- **LobbyManager**: Tracks which players are in which room to target the broadcast correctly.

## Error Handling
- **Invalid Packets**: Packets with incorrect CRC or smaller than expected size are dropped.
- **Permission Check**: A player cannot send a message to a room they haven't joined.
- **Cleanup**: Inactive sessions are cleaned up, and their associated players are removed from rooms.
