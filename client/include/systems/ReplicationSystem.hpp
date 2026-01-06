#pragma once

#include "ClientRuntime.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "graphics/abstraction/ISound.hpp"
#include "graphics/abstraction/ISoundBuffer.hpp"
#include "level/EntityTypeRegistry.hpp"
#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"
#include "network/SnapshotParser.hpp"
#include "systems/ISystem.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

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
    void applyLives(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyScore(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyStatusEffects(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyStatus(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyDead(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyInterpolation(Registry& registry, EntityId id, const SnapshotEntity& entity, std::uint32_t tickId);

    ThreadSafeQueue<SnapshotParseResult>* snapshots_;
    ThreadSafeQueue<EntitySpawnPacket>* spawnQueue_;
    ThreadSafeQueue<EntityDestroyedPacket>* destroyQueue_;
    const EntityTypeRegistry* types_;
    std::unordered_map<std::uint32_t, EntityId> remoteToLocal_;
    std::unordered_map<std::uint32_t, std::uint16_t> remoteToType_;
    std::unordered_map<std::uint32_t, std::uint32_t> lastSeenTick_;
    std::unordered_map<std::uint32_t, int> lastKnownLives_;
    std::unordered_map<std::uint32_t, float> respawnCooldown_;
    float explosionCooldown_ = 0.0F;

    std::shared_ptr<ISoundBuffer> laserBuffer_;
    std::shared_ptr<ISoundBuffer> explosionBuffer_;
    bool laserLoaded_        = false;
    bool laserLoadAttempted_ = false;
    std::vector<std::shared_ptr<ISound>> laserSounds_;
    std::vector<std::shared_ptr<ISound>> explosionSounds_;
};
