#pragma once

#include "ecs/Registry.hpp"
#include "network/InputPacket.hpp"

#include <cstdint>
#include <optional>

struct ServerInput
{
    EntityId playerId        = 0;
    std::uint16_t flags      = 0;
    float x                  = 0.0F;
    float y                  = 0.0F;
    float angle              = 0.0F;
    std::uint16_t sequenceId = 0;
    std::uint32_t tickId     = 0;
};

std::optional<ServerInput> parseInputPacket(const std::uint8_t* data, std::size_t len);
