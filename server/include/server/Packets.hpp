#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"
#include "network/PacketHeader.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct LevelArchetype
{
    std::uint16_t typeId;
    std::string spriteId;
    std::string animId;
    std::uint8_t layer;
};

struct LevelDefinition
{
    std::uint16_t levelId = 1;
    std::uint32_t seed    = 0;
    std::string backgroundId;
    std::string musicId;
    std::vector<LevelArchetype> archetypes;
};

std::vector<std::uint8_t> buildLevelInitPacket(const LevelDefinition& lvl);
std::vector<std::uint8_t> buildSnapshotPacket(Registry& registry, uint32_t tick);
std::vector<std::uint8_t> buildPong(const PacketHeader& req);
std::vector<std::uint8_t> buildServerHello(std::uint16_t sequence);
std::vector<std::uint8_t> buildJoinAccept(std::uint16_t sequence);
std::vector<std::uint8_t> buildGameStart(std::uint16_t sequence);
