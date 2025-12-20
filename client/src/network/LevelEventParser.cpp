#include "network/LevelEventParser.hpp"

#include "network/PacketHeader.hpp"

#include <bit>
#include <cmath>

namespace
{
    bool ensureAvailable(std::size_t offset, std::size_t need, std::size_t total)
    {
        return offset + need <= total;
    }

    std::uint8_t readU8(const std::vector<std::uint8_t>& buf, std::size_t& offset)
    {
        return buf[offset++];
    }

    std::uint32_t readU32(const std::vector<std::uint8_t>& buf, std::size_t& offset)
    {
        std::uint32_t val = (static_cast<std::uint32_t>(buf[offset]) << 24) |
                            (static_cast<std::uint32_t>(buf[offset + 1]) << 16) |
                            (static_cast<std::uint32_t>(buf[offset + 2]) << 8) |
                            static_cast<std::uint32_t>(buf[offset + 3]);
        offset += 4;
        return val;
    }

    float readF32(const std::vector<std::uint8_t>& buf, std::size_t& offset)
    {
        float v = std::bit_cast<float>(readU32(buf, offset));
        return v;
    }

    std::string readString(const std::vector<std::uint8_t>& buf, std::size_t& offset)
    {
        std::uint8_t len = buf[offset++];
        std::string str(buf.begin() + static_cast<long>(offset), buf.begin() + static_cast<long>(offset + len));
        offset += len;
        return str;
    }
}

std::optional<LevelEventData> LevelEventParser::parse(const std::vector<std::uint8_t>& data)
{
    if (data.size() < PacketHeader::kSize + PacketHeader::kCrcSize) {
        return std::nullopt;
    }

    auto hdr = PacketHeader::decode(data.data(), data.size());
    if (!hdr.has_value()) {
        return std::nullopt;
    }
    if (hdr->packetType != static_cast<std::uint8_t>(PacketType::ServerToClient)) {
        return std::nullopt;
    }
    if (hdr->messageType != static_cast<std::uint8_t>(MessageType::LevelEvent)) {
        return std::nullopt;
    }
    std::size_t payloadOffset = PacketHeader::kSize;
    std::size_t crcOffset     = payloadOffset + hdr->payloadSize;
    if (data.size() < crcOffset + PacketHeader::kCrcSize) {
        return std::nullopt;
    }
    std::uint32_t transmitted = (static_cast<std::uint32_t>(data[crcOffset]) << 24) |
                                (static_cast<std::uint32_t>(data[crcOffset + 1]) << 16) |
                                (static_cast<std::uint32_t>(data[crcOffset + 2]) << 8) |
                                static_cast<std::uint32_t>(data[crcOffset + 3]);
    std::uint32_t computed = PacketHeader::crc32(data.data(), crcOffset);
    if (transmitted != computed) {
        return std::nullopt;
    }

    std::size_t offset = payloadOffset;
    std::size_t total  = payloadOffset + hdr->payloadSize;
    if (!ensureAvailable(offset, 1, total)) {
        return std::nullopt;
    }

    LevelEventData out{};
    out.type = static_cast<LevelEventType>(readU8(data, offset));

    if (out.type == LevelEventType::SetScroll) {
        if (!ensureAvailable(offset, 1 + 4 + 1, total)) {
            return std::nullopt;
        }
        LevelScrollSettings settings;
        settings.mode   = static_cast<LevelScrollMode>(readU8(data, offset));
        settings.speedX = readF32(data, offset);
        if (!std::isfinite(settings.speedX)) {
            return std::nullopt;
        }
        std::uint8_t count = readU8(data, offset);
        settings.curve.reserve(count);
        for (std::uint8_t i = 0; i < count; ++i) {
            if (!ensureAvailable(offset, 8, total)) {
                return std::nullopt;
            }
            float time   = readF32(data, offset);
            float speedX = readF32(data, offset);
            if (!std::isfinite(time) || !std::isfinite(speedX)) {
                return std::nullopt;
            }
            settings.curve.push_back(LevelScrollKeyframe{time, speedX});
        }
        out.scroll = std::move(settings);
        return out;
    }

    if (out.type == LevelEventType::SetBackground) {
        if (!ensureAvailable(offset, 1, total)) {
            return std::nullopt;
        }
        std::uint8_t len = data[offset];
        if (!ensureAvailable(offset, 1 + len, total)) {
            return std::nullopt;
        }
        out.backgroundId = readString(data, offset);
        return out;
    }

    if (out.type == LevelEventType::SetMusic) {
        if (!ensureAvailable(offset, 1, total)) {
            return std::nullopt;
        }
        std::uint8_t len = data[offset];
        if (!ensureAvailable(offset, 1 + len, total)) {
            return std::nullopt;
        }
        out.musicId = readString(data, offset);
        return out;
    }

    if (out.type == LevelEventType::SetCameraBounds) {
        if (!ensureAvailable(offset, 16, total)) {
            return std::nullopt;
        }
        LevelCameraBounds bounds;
        bounds.minX = readF32(data, offset);
        bounds.maxX = readF32(data, offset);
        bounds.minY = readF32(data, offset);
        bounds.maxY = readF32(data, offset);
        if (!std::isfinite(bounds.minX) || !std::isfinite(bounds.maxX) || !std::isfinite(bounds.minY) ||
            !std::isfinite(bounds.maxY)) {
            return std::nullopt;
        }
        out.cameraBounds = bounds;
        return out;
    }

    if (out.type == LevelEventType::GateOpen || out.type == LevelEventType::GateClose) {
        if (!ensureAvailable(offset, 1, total)) {
            return std::nullopt;
        }
        std::uint8_t len = data[offset];
        if (!ensureAvailable(offset, 1 + len, total)) {
            return std::nullopt;
        }
        out.gateId = readString(data, offset);
        return out;
    }

    return std::nullopt;
}
