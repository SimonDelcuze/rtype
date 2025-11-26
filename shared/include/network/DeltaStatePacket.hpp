#pragma once

#include "network/PacketHeader.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <vector>

struct DeltaEntry
{
    std::uint32_t entityId = 0;
    float posX             = 0.0F;
    float posY             = 0.0F;
    float velX             = 0.0F;
    float velY             = 0.0F;
    std::int32_t hp        = 0;
};

struct DeltaStatePacket
{
    PacketHeader header{};
    std::vector<DeltaEntry> entries;

    std::vector<std::uint8_t> encode() const;
    static std::optional<DeltaStatePacket> decode(const std::uint8_t* data, std::size_t len);
};
