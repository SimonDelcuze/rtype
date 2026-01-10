#include "replication/ReplicationManager.hpp"

#include "network/Packets.hpp"

ReplicationManager::ReplicationManager() = default;

ReplicationManager::SyncResult ReplicationManager::synchronize(const Registry& registry, std::uint32_t currentTick)
{
    return synchronize(registry, currentTick, false);
}

ReplicationManager::SyncResult ReplicationManager::synchronize(const Registry& registry, std::uint32_t currentTick,
                                                               bool forceFull)
{
    std::vector<std::vector<std::uint8_t>> packets = buildSmartDeltaSnapshot(
        const_cast<Registry&>(registry), currentTick, entityStateCache_, forceFull, Network::kMaxSafePacketPayload);

    return SyncResult{std::move(packets), forceFull};
}

void ReplicationManager::clear()
{
    entityStateCache_.clear();
}
