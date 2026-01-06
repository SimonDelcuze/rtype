#include "network/SnapshotParser.hpp"

#include "Logger.hpp"
#include "network/NetworkCompression.hpp"
#include "network/Packing.hpp"

#include <bit>
#include <cstddef>
#include <string>

std::optional<SnapshotParseResult> SnapshotParser::parse(const std::vector<std::uint8_t>& data)
{
    PacketHeader header{};
    if (!validateHeader(data, header)) {
        return std::nullopt;
    }
    if (!validateSizes(data, header)) {
        return std::nullopt;
    }
    if (!validateCrc(data, header)) {
        return std::nullopt;
    }

    std::vector<std::uint8_t> actualData;
    const std::vector<std::uint8_t>* currentData = &data;
    PacketHeader currentHeader                   = header;

    if (header.isCompressed) {
        try {
            std::vector<std::uint8_t> decompressed =
                Compression::decompress(data.data() + PacketHeader::kSize, header.payloadSize, header.originalSize);

            actualData.reserve(PacketHeader::kSize + decompressed.size());
            auto hdrBytes = header.encode();
            actualData.insert(actualData.end(), hdrBytes.begin(), hdrBytes.end());
            actualData.insert(actualData.end(), decompressed.begin(), decompressed.end());

            currentData                = &actualData;
            currentHeader.isCompressed = false;
            currentHeader.payloadSize  = static_cast<std::uint16_t>(decompressed.size());
        } catch (const std::exception& e) {
            Logger::instance().error("[Net] Decompression failed: " + std::string(e.what()));
            return std::nullopt;
        }
    }

    std::size_t offset        = PacketHeader::kSize;
    std::uint16_t entityCount = readU16(*currentData, offset);

    SnapshotParseResult result{};
    result.header = currentHeader;
    result.entities.reserve(entityCount);

    for (std::uint16_t i = 0; i < entityCount; ++i) {
        auto entity = parseEntity(*currentData, offset, currentHeader);
        if (!entity.has_value()) {
            return std::nullopt;
        }
        result.entities.push_back(*entity);
    }
    return result;
}

bool SnapshotParser::hasBit(std::uint16_t mask, int bit)
{
    return (mask & (1u << bit)) != 0;
}

bool SnapshotParser::ensureAvailable(std::size_t offset, std::size_t need, std::size_t size)
{
    return offset + need <= size;
}

bool SnapshotParser::validateHeader(const std::vector<std::uint8_t>& data, PacketHeader& outHeader)
{
    if (data.size() < PacketHeader::kSize + PacketHeader::kCrcSize) {
        return false;
    }
    auto hdr = PacketHeader::decode(data.data(), data.size());
    if (!hdr) {
        return false;
    }
    if (hdr->packetType != static_cast<std::uint8_t>(PacketType::ServerToClient)) {
        return false;
    }
    if (hdr->messageType != static_cast<std::uint8_t>(MessageType::Snapshot)) {
        return false;
    }
    outHeader = *hdr;
    return true;
}

bool SnapshotParser::validateSizes(const std::vector<std::uint8_t>& data, const PacketHeader& header)
{
    const std::size_t payloadSize = header.payloadSize;
    const std::size_t expected    = PacketHeader::kSize + payloadSize + PacketHeader::kCrcSize;
    if (data.size() < expected) {
        Logger::instance().warn("[Net] Snapshot size mismatch: expected>=" + std::to_string(expected) +
                                " got=" + std::to_string(data.size()) + " payloadSize=" + std::to_string(payloadSize));
        return false;
    }
    return true;
}

bool SnapshotParser::validateCrc(const std::vector<std::uint8_t>& data, const PacketHeader& header)
{
    std::size_t payloadSize = header.payloadSize;
    std::size_t crcOffset   = PacketHeader::kSize + payloadSize;
    auto computedCrc        = PacketHeader::crc32(data.data(), crcOffset);
    std::uint32_t transmittedCrc =
        (static_cast<std::uint32_t>(data[crcOffset]) << 24) | (static_cast<std::uint32_t>(data[crcOffset + 1]) << 16) |
        (static_cast<std::uint32_t>(data[crcOffset + 2]) << 8) | static_cast<std::uint32_t>(data[crcOffset + 3]);
    return computedCrc == transmittedCrc;
}

std::uint16_t SnapshotParser::readU16(const std::vector<std::uint8_t>& buf, std::size_t& offset)
{
    std::uint16_t v = (static_cast<std::uint16_t>(buf[offset]) << 8) | static_cast<std::uint16_t>(buf[offset + 1]);
    offset += 2;
    return v;
}

std::uint32_t SnapshotParser::readU32(const std::vector<std::uint8_t>& buf, std::size_t& offset)
{
    std::uint32_t v = (static_cast<std::uint32_t>(buf[offset]) << 24) |
                      (static_cast<std::uint32_t>(buf[offset + 1]) << 16) |
                      (static_cast<std::uint32_t>(buf[offset + 2]) << 8) | static_cast<std::uint32_t>(buf[offset + 3]);
    offset += 4;
    return v;
}

std::int32_t SnapshotParser::readI32(const std::vector<std::uint8_t>& buf, std::size_t& offset)
{
    return static_cast<std::int32_t>(readU32(buf, offset));
}

float SnapshotParser::readFloat(const std::vector<std::uint8_t>& buf, std::size_t& offset)
{
    auto v = readU32(buf, offset);
    return std::bit_cast<float>(v);
}

std::optional<SnapshotEntity> SnapshotParser::parseEntity(const std::vector<std::uint8_t>& data, std::size_t& offset,
                                                          const PacketHeader& header)
{
    if (!ensureAvailable(offset, 6, PacketHeader::kSize + header.payloadSize)) {
        return std::nullopt;
    }
    SnapshotEntity e{};
    e.entityId   = readU32(data, offset);
    e.updateMask = readU16(data, offset);
    auto mask    = e.updateMask;

    if (hasBit(mask, 0)) {
        if (!ensureAvailable(offset, 1, PacketHeader::kSize + header.payloadSize))
            return std::nullopt;
        e.entityType = data[offset];
        offset += 1;
    }
    if (hasBit(mask, 1)) {
        if (!ensureAvailable(offset, 2, PacketHeader::kSize + header.payloadSize))
            return std::nullopt;
        e.posX = Packing::dequantizeFrom16(static_cast<std::int16_t>(readU16(data, offset)), 10.0F);
    }
    if (hasBit(mask, 2)) {
        if (!ensureAvailable(offset, 2, PacketHeader::kSize + header.payloadSize))
            return std::nullopt;
        e.posY = Packing::dequantizeFrom16(static_cast<std::int16_t>(readU16(data, offset)), 10.0F);
    }
    if (hasBit(mask, 3)) {
        if (!ensureAvailable(offset, 2, PacketHeader::kSize + header.payloadSize))
            return std::nullopt;
        e.velX = Packing::dequantizeFrom16(static_cast<std::int16_t>(readU16(data, offset)), 10.0F);
    }
    if (hasBit(mask, 4)) {
        if (!ensureAvailable(offset, 2, PacketHeader::kSize + header.payloadSize))
            return std::nullopt;
        e.velY = Packing::dequantizeFrom16(static_cast<std::int16_t>(readU16(data, offset)), 10.0F);
    }
    if (hasBit(mask, 5)) {
        if (!ensureAvailable(offset, 2, PacketHeader::kSize + header.payloadSize))
            return std::nullopt;
        std::uint16_t hv = readU16(data, offset);
        e.health         = static_cast<std::int16_t>(hv);
    }
    if (hasBit(mask, 6)) {
        if (!ensureAvailable(offset, 1, PacketHeader::kSize + header.payloadSize))
            return std::nullopt;
        std::uint8_t packed = data[offset];
        std::uint8_t status, lives;
        Packing::unpack44(packed, status, lives);
        e.statusEffects = status;
        e.lives         = static_cast<std::int8_t>(lives);
        offset += 1;
    }
    if (hasBit(mask, 7)) {
        if (!ensureAvailable(offset, 4, PacketHeader::kSize + header.payloadSize))
            return std::nullopt;
        e.orientation = readFloat(data, offset);
    }
    if (hasBit(mask, 8)) {
        if (!ensureAvailable(offset, 1, PacketHeader::kSize + header.payloadSize))
            return std::nullopt;
        e.dead = data[offset] != 0;
        offset += 1;
    }
    if (hasBit(mask, 10)) {
        if (!ensureAvailable(offset, 4, PacketHeader::kSize + header.payloadSize))
            return std::nullopt;
        e.score = readI32(data, offset);
    }
    return e;
}
