#include "server/Packets.hpp"

namespace
{
    void writeU16(std::vector<std::uint8_t>& out, std::uint16_t v)
    {
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    }

    void writeU32(std::vector<std::uint8_t>& out, std::uint32_t v)
    {
        out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    }

    void writeString(std::vector<std::uint8_t>& out, const std::string& s)
    {
        out.push_back(static_cast<std::uint8_t>(s.size()));
        out.insert(out.end(), s.begin(), s.end());
    }
} // namespace

std::vector<std::uint8_t> buildLevelInitPacket(const LevelDefinition& lvl)
{
    std::vector<std::uint8_t> payload;
    writeU16(payload, lvl.levelId);
    writeU32(payload, lvl.seed);
    writeString(payload, lvl.backgroundId);
    writeString(payload, lvl.musicId);
    payload.push_back(static_cast<std::uint8_t>(lvl.archetypes.size()));
    for (const auto& a : lvl.archetypes) {
        writeU16(payload, a.typeId);
        writeString(payload, a.spriteId);
        writeString(payload, a.animId);
        payload.push_back(a.layer);
    }

    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LevelInit);
    hdr.payloadSize = static_cast<std::uint16_t>(payload.size());
    auto hdrBytes   = hdr.encode();
    std::vector<std::uint8_t> out(hdrBytes.begin(), hdrBytes.end());
    out.insert(out.end(), payload.begin(), payload.end());
    auto crc = PacketHeader::crc32(out.data(), out.size());
    writeU32(out, crc);
    return out;
}

std::vector<std::uint8_t> buildPong(const PacketHeader& req)
{
    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::ServerPong);
    hdr.sequenceId  = req.sequenceId;
    hdr.tickId      = req.tickId;
    hdr.payloadSize = 0;
    auto hdrBytes   = hdr.encode();
    std::vector<std::uint8_t> out(hdrBytes.begin(), hdrBytes.end());
    auto crc = PacketHeader::crc32(out.data(), out.size());
    writeU32(out, crc);
    return out;
}

std::vector<std::uint8_t> buildServerHello(std::uint16_t sequence)
{
    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::ServerHello);
    hdr.sequenceId  = sequence;
    hdr.payloadSize = 0;
    auto hdrBytes   = hdr.encode();
    std::vector<std::uint8_t> out(hdrBytes.begin(), hdrBytes.end());
    auto crc = PacketHeader::crc32(out.data(), out.size());
    writeU32(out, crc);
    return out;
}

std::vector<std::uint8_t> buildJoinAccept(std::uint16_t sequence)
{
    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::ServerJoinAccept);
    hdr.sequenceId  = sequence;
    hdr.payloadSize = 0;
    auto hdrBytes   = hdr.encode();
    std::vector<std::uint8_t> out(hdrBytes.begin(), hdrBytes.end());
    auto crc = PacketHeader::crc32(out.data(), out.size());
    writeU32(out, crc);
    return out;
}

std::vector<std::uint8_t> buildGameStart(std::uint16_t sequence)
{
    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::GameStart);
    hdr.sequenceId  = sequence;
    hdr.payloadSize = 0;
    auto hdrBytes   = hdr.encode();
    std::vector<std::uint8_t> out(hdrBytes.begin(), hdrBytes.end());
    auto crc = PacketHeader::crc32(out.data(), out.size());
    writeU32(out, crc);
    return out;
}
