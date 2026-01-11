#Packet Header

All server and client UDP messages begin with a compact, fixed-size header.\
It provides essential metadata for validation, ordering, and synchronization, while keeping bandwidth usage low.

***

## **Structure**

Wire format: 7 bytes (big-endian)

| Field        | Type         | Size | Endianness | Description                                  |
|--------------|--------------|------|------------|----------------------------------------------|
| `messageType`| `uint8`      | 1    | N/A        | Logical packet kind (e.g., Snapshot, Input)  |
| `sequenceId` | `uint16`     | 2    | Big-endian | Monotonic per-peer counter (ordering/dedup)  |
| `tickId`     | `uint32`     | 4    | Big-endian | Authoritative server tick index               |

Notes:
- `messageType` uses a single byte;
unknown types are ignored by receivers.- `sequenceId` increases per peer and wraps naturally; stale or out-of-order packets can be detected.
- `tickId` references the simulation tick for snapshot payloads; for
    input packets, it may be set to the most recent tick known to the sender.

                               ***

                           ##**Encoding Rules * *

                           -Endianness : sequence and tick are encoded in big -
                       endian(network byte order).- Size : always 7 bytes on the wire; never padded.
- Platform neutrality: do not rely on in-memory struct layout for transmission.

***

## **C++ Definition**

Location: `shared/include/network/PacketHeader.hpp`

```cpp
#include <array>
#include <cstdint>
#include <optional>

enum class MessageType : std::uint8_t {
    Invalid   = 0,
    Snapshot  = 1, // Server -> Client
    Input     = 2, // Client -> Server
    Handshake = 3,
    Ack       = 4,
    PlayerDisconnected = 0x1C,
    EntitySpawn        = 0x1D,
    EntityDestroyed    = 0x1E
};

struct PacketHeader
{
    std::uint8_t messageType = static_cast<std::uint8_t>(MessageType::Invalid);
    std::uint16_t sequenceId = 0;
    std::uint32_t tickId     = 0;

    static constexpr std::size_t kSize = 7; // 1 + 2 + 4

    std::array<std::uint8_t, kSize> encode() const noexcept
    {
        std::array<std::uint8_t, kSize> out{};
        out[0] = messageType;
        out[1] = static_cast<std::uint8_t>((sequenceId >> 8) & 0xFF);
        out[2] = static_cast<std::uint8_t>(sequenceId & 0xFF);
        out[3] = static_cast<std::uint8_t>((tickId >> 24) & 0xFF);
        out[4] = static_cast<std::uint8_t>((tickId >> 16) & 0xFF);
        out[5] = static_cast<std::uint8_t>((tickId >> 8) & 0xFF);
        out[6] = static_cast<std::uint8_t>(tickId & 0xFF);
        return out;
    }

    static std::optional<PacketHeader> decode(const std::uint8_t* data, std::size_t len) noexcept
    {
        if (!data || len < kSize)
            return std::nullopt;
        PacketHeader h{};
        h.messageType = data[0];
        h.sequenceId  = static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[1]) << 8) |
                                                   static_cast<std::uint16_t>(data[2]));
        h.tickId      = (static_cast<std::uint32_t>(data[3]) << 24) | (static_cast<std::uint32_t>(data[4]) << 16) |
                   (static_cast<std::uint32_t>(data[5]) << 8) | static_cast<std::uint32_t>(data[6]);
        return h;
    }
};
```

    This approach avoids platform -
    specific headers(`winsock2.h`, `<arpa / inet.h>`) inside the
    public interface and guarantees identical wire encoding on Windows,
    Linux,
    and macOS.

                    ***

                ##**Usage Guidelines * *

                -Validate `messageType` and size before decoding the payload.-
            Maintain a per - client `sequenceId` to ignore stale
        or
        duplicated packets.- Treat `tickId` as the authoritative server timeline when processing snapshots.- Do not cast
        or
        memcpy the struct directly to the socket buffer—always use `encode()`/`decode()`.

                    * **

                       ## *
                    *Compatibility *
                    *

                    -Fixed
                - size wire header(7 bytes) ensures consistent behavior across compilers
            and platforms.- Big - endian encoding follows standard network byte order conventions.- The in
                    - memory `sizeof(PacketHeader)` is not relied upon;
only `PacketHeader::kSize` and the encode / decode functions define the wire format.
