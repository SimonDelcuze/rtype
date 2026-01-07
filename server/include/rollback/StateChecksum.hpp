#pragma once

#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

using EntityId = std::uint32_t;

struct CachedEntityState;

class StateChecksum
{
  public:
    static std::uint32_t compute(const std::unordered_map<EntityId, CachedEntityState>& entities);

    static std::uint32_t computeCritical(
        const std::unordered_map<EntityId, CachedEntityState>& entities);

    static bool verify(std::uint32_t checksum1, std::uint32_t checksum2)
    {
        return checksum1 == checksum2;
    }

  private:
    static const std::uint32_t CRC32_TABLE[256];

    static std::uint32_t updateCRC32(std::uint32_t crc, const void* data, std::size_t length);

    static std::uint32_t finalizeCRC32(std::uint32_t crc)
    {
        return ~crc;
    }
};
