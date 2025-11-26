#include "network/DeltaStatePacket.hpp"

#include <bit>

std::vector<std::uint8_t> DeltaStatePacket::encode() const
{
    PacketHeader h      = header;
    h.messageType       = static_cast<std::uint8_t>(MessageType::Snapshot);
    h.packetType        = static_cast<std::uint8_t>(PacketType::ServerToClient);
    std::uint16_t count = static_cast<std::uint16_t>(entries.size());
    h.payloadSize       = static_cast<std::uint16_t>(2 + count * 24);
    auto hdr            = h.encode();
    std::vector<std::uint8_t> out;
    out.reserve(PacketHeader::kSize + h.payloadSize);
    out.insert(out.end(), hdr.begin(), hdr.end());
    out.push_back(static_cast<std::uint8_t>((count >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>(count & 0xFF));
    auto w32 = [&](std::uint32_t v) {
        out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    };
    for (const auto& e : entries) {
        w32(e.entityId);
        w32(std::bit_cast<std::uint32_t>(e.posX));
        w32(std::bit_cast<std::uint32_t>(e.posY));
        w32(std::bit_cast<std::uint32_t>(e.velX));
        w32(std::bit_cast<std::uint32_t>(e.velY));
        w32(std::bit_cast<std::uint32_t>(static_cast<std::uint32_t>(e.hp)));
    }
    return out;
}

std::optional<DeltaStatePacket> DeltaStatePacket::decode(const std::uint8_t* data, std::size_t len)
{
    if (data == nullptr || len < PacketHeader::kSize + 2)
        return std::nullopt;
    auto hdrOpt = PacketHeader::decode(data, len);
    if (!hdrOpt)
        return std::nullopt;
    if (hdrOpt->messageType != static_cast<std::uint8_t>(MessageType::Snapshot))
        return std::nullopt;
    if (hdrOpt->payloadSize + PacketHeader::kSize != len)
        return std::nullopt;
    std::size_t offset = PacketHeader::kSize;
    if (hdrOpt->payloadSize < 2)
        return std::nullopt;
    std::uint16_t count = static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8) |
                                                     static_cast<std::uint16_t>(data[offset + 1]));
    offset += 2;
    std::size_t expectedPayload = 2 + static_cast<std::size_t>(count) * 24;
    if (hdrOpt->payloadSize != expectedPayload)
        return std::nullopt;
    DeltaStatePacket pkt;
    pkt.header = *hdrOpt;
    pkt.entries.reserve(count);
    auto r32 = [&](std::uint32_t& v) {
        v = (static_cast<std::uint32_t>(data[offset]) << 24) | (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
            (static_cast<std::uint32_t>(data[offset + 2]) << 8) | static_cast<std::uint32_t>(data[offset + 3]);
        offset += 4;
    };
    for (std::uint16_t i = 0; i < count; ++i) {
        DeltaEntry e{};
        std::uint32_t hpBits = 0;
        r32(e.entityId);
        std::uint32_t px{};
        std::uint32_t py{};
        std::uint32_t vx{};
        std::uint32_t vy{};
        r32(px);
        r32(py);
        r32(vx);
        r32(vy);
        r32(hpBits);
        e.posX = std::bit_cast<float>(px);
        e.posY = std::bit_cast<float>(py);
        e.velX = std::bit_cast<float>(vx);
        e.velY = std::bit_cast<float>(vy);
        e.hp   = static_cast<std::int32_t>(hpBits);
        pkt.entries.push_back(e);
    }
    return pkt;
}
