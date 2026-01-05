#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"
#include "network/LevelEventData.hpp"
#include "network/PacketHeader.hpp"
#include "server/EntityStateCache.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct SnapshotChunkBlock
{
    std::vector<std::uint8_t> data;
};

struct SnapshotChunkPacket
{
    std::vector<std::uint8_t> data;
    std::uint16_t entityCount = 0;
};

struct LevelArchetype
{
    std::uint16_t typeId;
    std::string spriteId;
    std::string animId;
    std::uint8_t layer;
};

struct LevelBossDefinition
{
    std::uint16_t typeId;
    std::string name;
    float scaleX = 1.0F;
    float scaleY = 1.0F;
};

struct LevelDefinition
{
    std::uint16_t levelId = 1;
    std::uint32_t seed    = 0;
    std::string backgroundId;
    std::string musicId;
    std::vector<LevelArchetype> archetypes;
    std::vector<LevelBossDefinition> bosses;
};

std::vector<std::uint8_t> buildLevelInitPacket(const LevelDefinition& lvl);
std::vector<std::uint8_t> buildLevelEventPacket(const LevelEventData& event, std::uint32_t tick);
std::vector<std::uint8_t> buildSnapshotPacket(Registry& registry, uint32_t tick);
std::vector<std::uint8_t> buildDeltaSnapshotPacket(Registry& registry, uint32_t tick,
                                                    EntityStateCache& cache, bool forceFullState);
std::vector<std::vector<std::uint8_t>> buildSnapshotChunks(Registry& registry, uint32_t tick,
                                                           std::size_t maxPayloadBytes = 1000);
std::vector<std::uint8_t> buildPong(const PacketHeader& req);
std::vector<std::uint8_t> buildServerHello(std::uint16_t sequence);
std::vector<std::uint8_t> buildJoinAccept(std::uint16_t sequence);
std::vector<std::uint8_t> buildJoinDeny(std::uint16_t sequence);
std::vector<std::uint8_t> buildGameStart(std::uint16_t sequence);
std::vector<std::uint8_t> buildAllReady(std::uint16_t sequence);
std::vector<std::uint8_t> buildCountdownTick(std::uint16_t sequence, std::uint8_t value);
