# Technical Guide: Network Compression Integration

This document explains how compression is integrated into the R-Type network stack and how to use it for new packet types.

## 1. Overview

RType uses **LZ4** for real-time packet compression. It is integrated directly into the `PacketHeader` and handled by `SnapshotParser` (client) and `SnapshotPackets` (server).

## 2. Compression Utility

The `Compression` namespace in `NetworkCompression.hpp` provides two simple functions:

```cpp
#include "network/NetworkCompression.hpp"

// Compression
std::vector<std::uint8_t> compressed = Compression::compress(rawPayload);

// Decompression
std::vector<std::uint8_t> decompressed = Compression::decompress(
    compressedDataPtr, 
    compressedLen, 
    originalSize
);
```

## 3. Protocol Integration

Compression is indicated in the `PacketHeader`:

1. **Flag**: The high bit (0x80) of the `version` field is set to 1.
2. **Payload Size**: `header.payloadSize` reflects the size of the *compressed* data.
3. **Original Size**: `header.originalSize` (bytes 15-16 of the header) stores the *decompressed* size.

## 4. How to use in New Packets

### Server Side (Sending)
When building a packet, use a threshold (e.g., 64 bytes) to decide whether to attempt compression.

```cpp
if (payload.size() > 64) {
    auto compressed = Compression::compress(payload);
    if (compressed.size() < payload.size()) {
        header.isCompressed = true;
        header.originalSize = payload.size();
        header.payloadSize = compressed.size();
        // ... build packet with compressed payload ...
    }
}
```

### Client Side (Receiving)
The `PacketHeader::decode` function automatically handles the bitmasking for the version and compression flag. You only need to check `header.isCompressed` and call `Compression::decompress`.

```cpp
if (header.isCompressed) {
    auto raw = Compression::decompress(data + kSize, header.payloadSize, header.originalSize);
    // ... parse 'raw' ...
}
```

## 5. Performance Considerations

- **LZ4** is chosen specifically for its extremely high decompression speed (multi-GB/s), ensuring that parsing snapshots doesn't block the main thread.
- Compression is only used if it actually reduces the packet size, avoiding the overhead for already tight payloads.
