#include "network/Packets.hpp"

#include <bit>

namespace
{
    void writeU16(std::vector<std::uint8_t>& out, std::uint16_t v)
    {
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    }

    void writeU8(std::vector<std::uint8_t>& out, std::uint8_t v)
    {
        out.push_back(v);
    }

    void writeU32(std::vector<std::uint8_t>& out, std::uint32_t v)
    {
        out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    }

    void writeF32(std::vector<std::uint8_t>& out, float v)
    {
        writeU32(out, std::bit_cast<std::uint32_t>(v));
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
    payload.push_back(static_cast<std::uint8_t>(lvl.bosses.size()));
    for (const auto& b : lvl.bosses) {
        writeU16(payload, b.typeId);
        writeString(payload, b.name);
        writeF32(payload, b.scaleX);
        writeF32(payload, b.scaleY);
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

std::vector<std::uint8_t> buildLevelEventPacket(const LevelEventData& event, std::uint32_t tick)
{
    std::vector<std::uint8_t> payload;
    writeU8(payload, static_cast<std::uint8_t>(event.type));

    if (event.type == LevelEventType::SetScroll) {
        if (!event.scroll.has_value())
            return {};
        writeU8(payload, static_cast<std::uint8_t>(event.scroll->mode));
        writeF32(payload, event.scroll->speedX);
        std::uint8_t count = static_cast<std::uint8_t>(event.scroll->curve.size());
        writeU8(payload, count);
        for (const auto& key : event.scroll->curve) {
            writeF32(payload, key.time);
            writeF32(payload, key.speedX);
        }
    } else if (event.type == LevelEventType::SetBackground) {
        if (!event.backgroundId.has_value())
            return {};
        writeString(payload, *event.backgroundId);
    } else if (event.type == LevelEventType::SetMusic) {
        if (!event.musicId.has_value())
            return {};
        writeString(payload, *event.musicId);
    } else if (event.type == LevelEventType::SetCameraBounds) {
        if (!event.cameraBounds.has_value())
            return {};
        writeF32(payload, event.cameraBounds->minX);
        writeF32(payload, event.cameraBounds->maxX);
        writeF32(payload, event.cameraBounds->minY);
        writeF32(payload, event.cameraBounds->maxY);
    } else if (event.type == LevelEventType::GateOpen || event.type == LevelEventType::GateClose) {
        if (!event.gateId.has_value())
            return {};
        writeString(payload, *event.gateId);
    } else {
        return {};
    }

    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LevelEvent);
    hdr.tickId      = tick;
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

std::vector<std::uint8_t> buildJoinDeny(std::uint16_t sequence)
{
    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::ServerJoinDeny);
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

std::vector<std::uint8_t> buildAllReady(std::uint16_t sequence)
{
    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::AllReady);
    hdr.sequenceId  = sequence;
    hdr.payloadSize = 0;
    auto hdrBytes   = hdr.encode();
    std::vector<std::uint8_t> out(hdrBytes.begin(), hdrBytes.end());
    auto crc = PacketHeader::crc32(out.data(), out.size());
    writeU32(out, crc);
    return out;
}

std::vector<std::uint8_t> buildCountdownTick(std::uint16_t sequence, std::uint8_t value)
{
    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::CountdownTick);
    hdr.sequenceId  = sequence;
    hdr.payloadSize = 1;
    auto hdrBytes   = hdr.encode();
    std::vector<std::uint8_t> out(hdrBytes.begin(), hdrBytes.end());
    out.push_back(value);
    auto crc = PacketHeader::crc32(out.data(), out.size());
    writeU32(out, crc);
    return out;
}
