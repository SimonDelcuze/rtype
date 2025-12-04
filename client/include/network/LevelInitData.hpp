#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ArchetypeEntry
{
    std::uint16_t typeId = 0;
    std::string spriteId;
    std::string animId;
    std::uint8_t layer = 0;
};

struct LevelInitData
{
    std::uint16_t levelId = 0;
    std::uint32_t seed    = 0;
    std::string backgroundId;
    std::string musicId;
    std::vector<ArchetypeEntry> archetypes;
};
