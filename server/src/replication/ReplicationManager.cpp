#include "replication/ReplicationManager.hpp"

#include "network/Packets.hpp"

ReplicationManager::ReplicationManager() = default;

ReplicationManager::SyncResult ReplicationManager::synchronize(const Registry& registry, std::uint32_t currentTick)
{
    std::vector<std::vector<std::uint8_t>> packets = buildSmartDeltaSnapshot(
        const_cast<Registry&>(registry), currentTick, entityStateCache_, false, kMaxPacketSize);

    return SyncResult{std::move(packets), false};
}

void ReplicationManager::clear()
{
    entityStateCache_.clear();
}
