#pragma once

#include "network/PacketHeader.hpp"

#include <cstdint>
#include <optional>
#include <vector>

struct SnapshotEntity
{
    std::uint32_t entityId = 0;
    std::uint16_t updateMask = 0;
    std::optional<std::uint8_t> entityType;
    std::optional<float> posX;
    std::optional<float> posY;
    std::optional<float> velX;
    std::optional<float> velY;
    std::optional<std::int16_t> health;
    std::optional<std::uint8_t> statusEffects;
    std::optional<float> orientation;
    std::optional<bool> dead;
};

struct SnapshotParseResult
{
    PacketHeader header;
    std::vector<SnapshotEntity> entities;
};

class SnapshotParser
{
  public:
    static std::optional<SnapshotParseResult> parse(const std::vector<std::uint8_t>& data);

  private:
    static bool hasBit(std::uint16_t mask, int bit);
    static bool ensureAvailable(std::size_t offset, std::size_t need, std::size_t size);
    static bool validateHeader(const std::vector<std::uint8_t>& data, PacketHeader& outHeader);
    static bool validateCrc(const std::vector<std::uint8_t>& data, const PacketHeader& header);
    static bool validateSizes(const std::vector<std::uint8_t>& data, const PacketHeader& header);
    static std::uint16_t readU16(const std::vector<std::uint8_t>& buf, std::size_t& offset);
    static std::uint32_t readU32(const std::vector<std::uint8_t>& buf, std::size_t& offset);
    static float readFloat(const std::vector<std::uint8_t>& buf, std::size_t& offset);
    static std::optional<SnapshotEntity> parseEntity(const std::vector<std::uint8_t>& data, std::size_t& offset, const PacketHeader& header);
};
