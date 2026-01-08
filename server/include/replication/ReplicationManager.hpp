#pragma once

#include "ecs/Registry.hpp"
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

    void clear();

  private:
    static constexpr std::uint32_t kFullStateInterval = 60;
    static constexpr std::size_t kMaxPacketSize       = 1400;

    EntityStateCache entityStateCache_;
    std::uint32_t lastFullStateTick_{0};
};
