#include "network/LevelInitParser.hpp"

bool LevelInitParser::validateHeader(const std::vector<std::uint8_t>& data)
{
    if (data.size() < PacketHeader::kSize) {
        return false;
    }
    auto hdr = PacketHeader::decode(data.data(), data.size());
    if (!hdr) {
        return false;
    }
    if (hdr->packetType != static_cast<std::uint8_t>(PacketType::ServerToClient)) {
        return false;
    }
    if (hdr->messageType != static_cast<std::uint8_t>(MessageType::LevelInit)) {
        return false;
    }
    return true;
}

bool LevelInitParser::ensureAvailable(std::size_t offset, std::size_t need, std::size_t total)
{
    return offset + need <= total;
}

std::uint8_t LevelInitParser::readU8(const std::vector<std::uint8_t>& buf, std::size_t& offset)
{
    return buf[offset++];
}

std::uint16_t LevelInitParser::readU16(const std::vector<std::uint8_t>& buf, std::size_t& offset)
{
    std::uint16_t val = static_cast<std::uint16_t>(buf[offset] << 8) | buf[offset + 1];
    offset += 2;
    return val;
}

std::uint32_t LevelInitParser::readU32(const std::vector<std::uint8_t>& buf, std::size_t& offset)
{
    std::uint32_t val = (static_cast<std::uint32_t>(buf[offset]) << 24) |
                        (static_cast<std::uint32_t>(buf[offset + 1]) << 16) |
                        (static_cast<std::uint32_t>(buf[offset + 2]) << 8) |
                        static_cast<std::uint32_t>(buf[offset + 3]);
    offset += 4;
    return val;
}

std::string LevelInitParser::readString(const std::vector<std::uint8_t>& buf, std::size_t& offset)
{
    std::uint8_t len = buf[offset++];
    std::string str(buf.begin() + static_cast<long>(offset),
                    buf.begin() + static_cast<long>(offset + len));
    offset += len;
    return str;
}

std::optional<ArchetypeEntry> LevelInitParser::parseArchetype(const std::vector<std::uint8_t>& buf,
                                                              std::size_t& offset,
                                                              std::size_t total)
{
    if (!ensureAvailable(offset, 2, total)) {
        return std::nullopt;
    }
    ArchetypeEntry entry{};
    entry.typeId = readU16(buf, offset);

    if (!ensureAvailable(offset, 1, total)) {
        return std::nullopt;
    }
    std::uint8_t spriteLen = buf[offset];
    if (!ensureAvailable(offset, 1 + spriteLen, total)) {
        return std::nullopt;
    }
    entry.spriteId = readString(buf, offset);

    if (!ensureAvailable(offset, 1, total)) {
        return std::nullopt;
    }
    std::uint8_t animLen = buf[offset];
    if (!ensureAvailable(offset, 1 + animLen, total)) {
        return std::nullopt;
    }
    entry.animId = readString(buf, offset);

    if (!ensureAvailable(offset, 1, total)) {
        return std::nullopt;
    }
    entry.layer = readU8(buf, offset);

    return entry;
}

std::optional<LevelInitData> LevelInitParser::parse(const std::vector<std::uint8_t>& data)
{
    if (!validateHeader(data)) {
        return std::nullopt;
    }

    std::size_t offset = PacketHeader::kSize;
    std::size_t total  = data.size();

    if (!ensureAvailable(offset, 6, total)) {
        return std::nullopt;
    }

    LevelInitData result{};
    result.levelId = readU16(data, offset);
    result.seed    = readU32(data, offset);

    if (!ensureAvailable(offset, 1, total)) {
        return std::nullopt;
    }
    std::uint8_t bgLen = data[offset];
    if (!ensureAvailable(offset, 1 + bgLen, total)) {
        return std::nullopt;
    }
    result.backgroundId = readString(data, offset);

    if (!ensureAvailable(offset, 1, total)) {
        return std::nullopt;
    }
    std::uint8_t musicLen = data[offset];
    if (!ensureAvailable(offset, 1 + musicLen, total)) {
        return std::nullopt;
    }
    result.musicId = readString(data, offset);

    if (!ensureAvailable(offset, 1, total)) {
        return std::nullopt;
    }
    std::uint8_t archetypeCount = readU8(data, offset);
    result.archetypes.reserve(archetypeCount);

    for (std::uint8_t i = 0; i < archetypeCount; ++i) {
        auto arch = parseArchetype(data, offset, total);
        if (!arch) {
            return std::nullopt;
        }
        result.archetypes.push_back(std::move(*arch));
    }

    return result;
}
