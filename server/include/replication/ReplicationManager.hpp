#pragma once

#include "ecs/Registry.hpp"
#include "network/NetworkConstants.hpp"
#include "replication/EntityStateCache.hpp"

#include <cstdint>
#include <unordered_set>
#include <vector>

class ReplicationManager
{
  public:
    ReplicationManager();

    struct SyncResult
    {
        std::vector<std::vector<std::uint8_t>> packets;
        bool wasFull;
    };

    SyncResult synchronize(const Registry& registry, std::uint32_t currentTick);
    SyncResult synchronize(const Registry& registry, std::uint32_t currentTick, bool forceFull);

    void clear();

  private:
    EntityStateCache entityStateCache_;
};
