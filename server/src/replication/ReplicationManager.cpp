#include "replication/ReplicationManager.hpp"

#include "network/Packets.hpp"

ReplicationManager::ReplicationManager() = default;

ReplicationManager::SyncResult ReplicationManager::synchronize(const Registry& registry, std::uint32_t currentTick)
{
    bool forceFull = (currentTick - lastFullStateTick_) >= kFullStateInterval;
    if (forceFull) {
        lastFullStateTick_ = currentTick;
    }

    std::vector<std::vector<std::uint8_t>> packets;

    if (forceFull) {
        packets = buildSnapshotChunks(const_cast<Registry&>(registry), currentTick);
    } else {
        packets = buildSmartDeltaSnapshot(const_cast<Registry&>(registry), currentTick, entityStateCache_, false,
                                          kMaxPacketSize);
    }

    return SyncResult{std::move(packets), forceFull};
}

void ReplicationManager::clear()
{
    entityStateCache_.clear();
    lastFullStateTick_ = 0;
}
