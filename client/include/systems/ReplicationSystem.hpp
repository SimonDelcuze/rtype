#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "level/EntityTypeRegistry.hpp"
#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"
#include "network/SnapshotParser.hpp"
#include "systems/ISystem.hpp"

#include <optional>
#include <unordered_map>

class ReplicationSystem : public ISystem
{
  public:
    ReplicationSystem(ThreadSafeQueue<SnapshotParseResult>& snapshots, ThreadSafeQueue<EntitySpawnPacket>& spawns,
                      ThreadSafeQueue<EntityDestroyedPacket>& destroys, const EntityTypeRegistry& types);
    ReplicationSystem(ThreadSafeQueue<SnapshotParseResult>& snapshots, const EntityTypeRegistry& types);

    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;

  private:
    std::optional<EntityId> ensureEntity(Registry& registry, const SnapshotEntity& entity);
    void applyArchetype(Registry& registry, EntityId id, std::uint16_t typeId);
    void applyEntity(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyTransform(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyVelocity(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyHealth(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyStatus(Registry&, EntityId, const SnapshotEntity&);
    void applyDead(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyInterpolation(Registry& registry, EntityId id, const SnapshotEntity& entity, std::uint32_t tickId);

    ThreadSafeQueue<SnapshotParseResult>* snapshots_;
    ThreadSafeQueue<EntitySpawnPacket>* spawnQueue_;
    ThreadSafeQueue<EntityDestroyedPacket>* destroyQueue_;
    const EntityTypeRegistry* types_;
    std::unordered_map<std::uint32_t, EntityId> remoteToLocal_;
};
