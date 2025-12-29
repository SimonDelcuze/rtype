#include "systems/ReplicationSystem.hpp"

#include "Logger.hpp"
#include "animation/AnimationRegistry.hpp"
#include "components/AnimationComponent.hpp"
#include "components/ColliderComponent.hpp"
#include "components/DirectionalAnimationComponent.hpp"
#include "components/HealthComponent.hpp"
#include "components/InterpolationComponent.hpp"
#include "components/InvincibilityComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/LivesComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TagComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"
#include "ecs/Registry.hpp"
#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <unordered_set>

ReplicationSystem::ReplicationSystem(ThreadSafeQueue<SnapshotParseResult>& snapshots,
                                     ThreadSafeQueue<EntitySpawnPacket>& spawns,
                                     ThreadSafeQueue<EntityDestroyedPacket>& destroys, const EntityTypeRegistry& types)
    : snapshots_(&snapshots), spawnQueue_(&spawns), destroyQueue_(&destroys), types_(&types)
{}

namespace
{
    bool loadBufferWithFallback(sf::SoundBuffer& buffer, bool& attempted, bool& loaded, const std::string& primary,
                                const std::string& fallback)
    {
        if (loaded) {
            return loaded;
        }
        attempted = true;
        if (buffer.loadFromFile(primary)) {
            loaded = true;
            return true;
        }
        if (!fallback.empty() && buffer.loadFromFile(fallback)) {
            loaded = true;
            return true;
        }
        return false;
    }

    ThreadSafeQueue<EntitySpawnPacket>& dummySpawnQueue()
    {
        static ThreadSafeQueue<EntitySpawnPacket> q;
        return q;
    }
    ThreadSafeQueue<EntityDestroyedPacket>& dummyDestroyQueue()
    {
        static ThreadSafeQueue<EntityDestroyedPacket> q;
        return q;
    }

    void ensurePool(std::vector<sf::Sound>& pool, const sf::SoundBuffer& buffer, std::size_t minSize)
    {
        if (pool.size() < minSize) {
            pool.reserve(minSize);
            for (std::size_t i = pool.size(); i < minSize; ++i) {
                sf::Sound s(buffer);
                pool.push_back(std::move(s));
            }
        }
    }
} // namespace

ReplicationSystem::ReplicationSystem(ThreadSafeQueue<SnapshotParseResult>& snapshots, const EntityTypeRegistry& types)
    : snapshots_(&snapshots), spawnQueue_(&dummySpawnQueue()), destroyQueue_(&dummyDestroyQueue()), types_(&types)
{}

namespace
{
    std::pair<float, float> defaultScaleForType(std::uint16_t typeId)
    {
        if (typeId == 9)
            return {2.0F, 2.0F};
        if (typeId == 11)
            return {3.0F, 3.0F};
        return {1.0F, 1.0F};
    }
} // namespace

void ReplicationSystem::initialize() {}

void ReplicationSystem::update(Registry& registry, float deltaTime)
{
    static std::uint32_t lastSnapshotTick = 0;
    static auto lastSnapshotTime          = std::chrono::steady_clock::now();
    static auto nextStaleLog              = std::chrono::steady_clock::now();

    for (auto it = respawnCooldown_.begin(); it != respawnCooldown_.end();) {
        it->second -= deltaTime;
        if (it->second <= 0.0F) {
            it = respawnCooldown_.erase(it);
        } else {
            ++it;
        }
    }

    EntitySpawnPacket spawnPkt;
    while (spawnQueue_->tryPop(spawnPkt)) {
        if (!types_->has(spawnPkt.entityType)) {
            Logger::instance().warn("[Replication] Unknown type in spawn: " + std::to_string(spawnPkt.entityType));
            continue;
        }
        auto existing = remoteToLocal_.find(spawnPkt.entityId);
        if (existing != remoteToLocal_.end()) {
            if (registry.isAlive(existing->second)) {
                registry.destroyEntity(existing->second);
            }
            Logger::instance().info("[Replication] Replacing existing entityId=" + std::to_string(spawnPkt.entityId) +
                                    " type=" + std::to_string(spawnPkt.entityType));
            remoteToLocal_.erase(existing);
        }
        EntityId id                       = registry.createEntity();
        remoteToLocal_[spawnPkt.entityId] = id;
        remoteToType_[spawnPkt.entityId]  = spawnPkt.entityType;
        applyArchetype(registry, id, spawnPkt.entityType);
        TransformComponent t{};
        auto [sx, sy] = defaultScaleForType(spawnPkt.entityType);
        t.scaleX      = sx;
        t.scaleY      = sy;
        t.x           = spawnPkt.posX;
        t.y           = spawnPkt.posY;
        registry.emplace<TransformComponent>(id, t);
        Logger::instance().info("[Replication] Spawn entityId=" + std::to_string(spawnPkt.entityId) +
                                " type=" + std::to_string(spawnPkt.entityType) + " local=" + std::to_string(id) +
                                " pos=(" + std::to_string(t.x) + "," + std::to_string(t.y) + ")");

        bool playerReadyToFire = false;
        for (const auto& kv : remoteToType_) {
            if (kv.second != 1)
                continue;
            auto itLocal = remoteToLocal_.find(kv.first);
            if (itLocal == remoteToLocal_.end() || !registry.isAlive(itLocal->second))
                continue;
            if (registry.has<InvincibilityComponent>(itLocal->second))
                continue;
            auto coolIt = respawnCooldown_.find(kv.first);
            if (coolIt != respawnCooldown_.end() && coolIt->second > 0.0F)
                continue;
            if (!registry.has<LivesComponent>(itLocal->second) ||
                registry.get<LivesComponent>(itLocal->second).current > 0) {
                playerReadyToFire = true;
                break;
            }
        }

        if (spawnPkt.entityType == 3) {
            loadBufferWithFallback(laserBuffer_, laserLoadAttempted_, laserLoaded_, "client/assets/sounds/laser.wav",
                                   "sounds/laser.wav");
            if (!laserLoaded_ && laserLoadAttempted_) {
                Logger::instance().warn(
                    "[Audio] Failed to load laser sound (tried client/assets/sounds/laser.wav and sounds/laser.wav)");
            }
            if (laserLoaded_ && playerReadyToFire) {
                ensurePool(laserSounds_, laserBuffer_, 6);
                bool played = false;
                for (auto& sound : laserSounds_) {
                    if (sound.getStatus() != sf::Sound::Status::Playing) {
                        sound.setBuffer(laserBuffer_);
                        sound.setVolume(std::clamp(g_musicVolume, 0.0F, 100.0F));
                        sound.play();
                        played = true;
                        break;
                    }
                }
                if (!played) {
                    if (laserSounds_.size() < 32) {
                        sf::Sound s(laserBuffer_);
                        s.setVolume(std::clamp(g_musicVolume, 0.0F, 100.0F));
                        s.play();
                        laserSounds_.push_back(std::move(s));
                    } else {
                        laserSounds_[0].stop();
                        laserSounds_[0].setBuffer(laserBuffer_);
                        laserSounds_[0].setVolume(std::clamp(g_musicVolume, 0.0F, 100.0F));
                        laserSounds_[0].play();
                    }
                }
            }
        }
    }

    EntityDestroyedPacket destroyPkt;
    while (destroyQueue_->tryPop(destroyPkt)) {
        auto it = remoteToLocal_.find(destroyPkt.entityId);
        if (it != remoteToLocal_.end()) {
            auto typeIt = remoteToType_.find(destroyPkt.entityId);
            if (typeIt != remoteToType_.end() && typeIt->second == 2) {
                loadBufferWithFallback(explosionBuffer_, explosionLoadTried_, explosionLoaded_,
                                       "client/assets/sounds/explosion.wav", "sounds/explosion.wav");
                if (!explosionLoaded_ && explosionLoadTried_) {
                    Logger::instance().warn("[Audio] Failed to load explosion sound (tried "
                                            "client/assets/sounds/explosion.wav and sounds/explosion.wav)");
                }
                if (explosionLoaded_) {
                    explosionSounds_.erase(
                        std::remove_if(explosionSounds_.begin(), explosionSounds_.end(),
                                       [](const sf::Sound& s) { return s.getStatus() == sf::Sound::Status::Stopped; }),
                        explosionSounds_.end());
                    bool played = false;
                    for (auto& sound : explosionSounds_) {
                        if (sound.getStatus() != sf::Sound::Status::Playing) {
                            sound.setBuffer(explosionBuffer_);
                            sound.setVolume(std::clamp(g_musicVolume, 0.0F, 100.0F));
                            sound.play();
                            played = true;
                            break;
                        }
                    }
                    if (!played) {
                        if (explosionSounds_.size() < 32) {
                            sf::Sound s(explosionBuffer_);
                            s.setVolume(std::clamp(g_musicVolume, 0.0F, 100.0F));
                            s.play();
                            explosionSounds_.push_back(std::move(s));
                        } else {
                            explosionSounds_[0].stop();
                            explosionSounds_[0].setBuffer(explosionBuffer_);
                            explosionSounds_[0].setVolume(std::clamp(g_musicVolume, 0.0F, 100.0F));
                            explosionSounds_[0].play();
                        }
                    }
                }
            }
            if (registry.isAlive(it->second)) {
                registry.destroyEntity(it->second);
            }
            remoteToLocal_.erase(it);
            remoteToType_.erase(destroyPkt.entityId);
            Logger::instance().info("[Replication] Destroy entityId=" + std::to_string(destroyPkt.entityId));
        }
    }

    SnapshotParseResult snapshot;
    while (snapshots_->tryPop(snapshot)) {
        lastSnapshotTick = snapshot.header.tickId;
        lastSnapshotTime = std::chrono::steady_clock::now();

        std::unordered_set<std::uint32_t> seenThisTick;
        seenThisTick.reserve(snapshot.entities.size());

        for (const auto& entity : snapshot.entities) {
            auto localId = ensureEntity(registry, entity);
            if (!localId.has_value()) {
                continue;
            }
            seenThisTick.insert(entity.entityId);
            if (entity.entityType.has_value())
                remoteToType_[entity.entityId] = *entity.entityType;
            lastSeenTick_[entity.entityId] = snapshot.header.tickId;
            applyEntity(registry, *localId, entity);
            applyStatusEffects(registry, *localId, entity);
            if (!registry.isAlive(*localId)) {
                continue;
            }
            applyInterpolation(registry, *localId, entity, snapshot.header.tickId);
        }

        for (auto it = remoteToLocal_.begin(); it != remoteToLocal_.end();) {
            if (seenThisTick.contains(it->first)) {
                ++it;
                continue;
            }
            if (registry.isAlive(it->second)) {
                registry.destroyEntity(it->second);
            }
            lastSeenTick_.erase(it->first);
            remoteToType_.erase(it->first);
            it = remoteToLocal_.erase(it);
        }
    }

    const std::uint32_t staleThreshold = lastSnapshotTick > 5 ? lastSnapshotTick - 5 : 0;
    for (auto it = remoteToLocal_.begin(); it != remoteToLocal_.end();) {
        auto seenIt = lastSeenTick_.find(it->first);
        if (seenIt != lastSeenTick_.end() && seenIt->second < staleThreshold) {
            if (registry.isAlive(it->second)) {
                registry.destroyEntity(it->second);
            }
            lastSeenTick_.erase(seenIt);
            remoteToType_.erase(it->first);
            it = remoteToLocal_.erase(it);
            continue;
        }
        ++it;
    }

    auto now = std::chrono::steady_clock::now();
    if (now - lastSnapshotTime > std::chrono::seconds(1) && now >= nextStaleLog) {
        Logger::instance().warn(
            "[Replication] No snapshots for " +
            std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSnapshotTime).count()) +
            "ms lastTick=" + std::to_string(lastSnapshotTick) + " entities=" + std::to_string(registry.entityCount()) +
            " mapped=" + std::to_string(remoteToLocal_.size()));
        nextStaleLog = now + std::chrono::seconds(1);
    }
}

void ReplicationSystem::cleanup()
{
    remoteToLocal_.clear();
}

std::optional<EntityId> ReplicationSystem::ensureEntity(Registry& registry, const SnapshotEntity& entity)
{
    auto remoteId = entity.entityId;
    auto it       = remoteToLocal_.find(remoteId);
    if (it != remoteToLocal_.end() && registry.isAlive(it->second)) {
        return it->second;
    }

    if (!entity.entityType.has_value()) {
        Logger::instance().warn("[Replication] Missing entityType for remoteId " + std::to_string(remoteId));
        return std::nullopt;
    }
    if (!types_->has(*entity.entityType)) {
        Logger::instance().warn("[Replication] Unknown entityType " +
                                std::to_string(static_cast<int>(*entity.entityType)) + " for remoteId " +
                                std::to_string(remoteId));
        return std::nullopt;
    }

    EntityId id              = registry.createEntity();
    remoteToLocal_[remoteId] = id;
    remoteToType_[remoteId]  = *entity.entityType;
    applyArchetype(registry, id, *entity.entityType);
    return std::optional<EntityId>(id);
}

void ReplicationSystem::applyArchetype(Registry& registry, EntityId id, std::uint16_t typeId)
{
    const auto* data = types_->get(typeId);
    if (data == nullptr) {
        return;
    }
    const bool isPlayer = (typeId == 1 || typeId == 12 || typeId == 13 || typeId == 14);
    const AnimationClip* clip =
        data->animation != nullptr ? reinterpret_cast<const AnimationClip*>(data->animation) : nullptr;
    if (data->texture != nullptr) {
        SpriteComponent sprite(*data->texture);
        if (clip != nullptr) {
            sprite.customFrames.clear();
            sprite.customFrames.reserve(clip->frames.size());
            for (const auto& f : clip->frames) {
                sprite.customFrames.emplace_back(sf::Vector2i{f.x, f.y}, sf::Vector2i{f.width, f.height});
            }
            if (!sprite.customFrames.empty()) {
                sprite.setFrame(0);
            }
        }
        if (data->frameWidth > 0 && data->frameHeight > 0) {
            sprite.setFrameSize(data->frameWidth, data->frameHeight, data->columns == 0 ? 1 : data->columns);
        }
        registry.emplace<SpriteComponent>(id, sprite);
    } else {
        registry.emplace<SpriteComponent>(id, SpriteComponent{});
    }
    if (clip != nullptr) {
        auto anim =
            AnimationComponent::create(static_cast<std::uint32_t>(clip->frames.size()), clip->frameTime, clip->loop);
        registry.emplace<AnimationComponent>(id, anim);
    }
    if (isPlayer && !registry.has<DirectionalAnimationComponent>(id)) {
        DirectionalAnimationComponent dir{};
        dir.spriteId = "player_ship";
        if (typeId == 1) {
            dir.idleLabel = "row1_idle";
            dir.upLabel   = "row1_up";
            dir.downLabel = "row1_down";
        } else if (typeId == 12) {
            dir.idleLabel = "row2_idle";
            dir.upLabel   = "row2_up";
            dir.downLabel = "row2_down";
        } else if (typeId == 13) {
            dir.idleLabel = "row3_idle";
            dir.upLabel   = "row3_up";
            dir.downLabel = "row3_down";
        } else {
            dir.idleLabel = "row4_idle";
            dir.upLabel   = "row4_up";
            dir.downLabel = "row4_down";
        }
        dir.threshold = 60.0F;
        registry.emplace<DirectionalAnimationComponent>(id, dir);
    }
    registry.emplace<LayerComponent>(id, LayerComponent::create(static_cast<int>(data->layer)));
    if (!registry.has<TagComponent>(id)) {
        EntityTag tag = EntityTag::Enemy;
        if (typeId == 1) {
            tag = EntityTag::Player;
        } else if (typeId == 9 || typeId == 10 || typeId == 11) {
            tag = EntityTag::Obstacle;
        } else if (typeId >= 3 && typeId <= 8) {
            tag = EntityTag::Projectile;
        } else if (typeId == 16) {
            tag = EntityTag::Background;
        }
        registry.emplace<TagComponent>(id, TagComponent::create(tag));
    }
    if ((typeId == 9 || typeId == 10 || typeId == 11) && !registry.has<ColliderComponent>(id)) {
        static const std::vector<std::array<float, 2>> topHull{{{0.0F, 0.0F},
                                                                {146.0F, 0.0F},
                                                                {146.0F, 4.0F},
                                                                {144.0F, 7.0F},
                                                                {139.0F, 14.0F},
                                                                {137.0F, 16.0F},
                                                                {129.0F, 22.0F},
                                                                {24.0F, 22.0F},
                                                                {4.0F, 6.0F},
                                                                {0.0F, 2.0F}}};
        static const std::vector<std::array<float, 2>> midHull{{{0.0F, 24.0F},
                                                                {2.0F, 20.0F},
                                                                {8.0F, 10.0F},
                                                                {10.0F, 8.0F},
                                                                {19.0F, 2.0F},
                                                                {21.0F, 1.0F},
                                                                {72.0F, 1.0F},
                                                                {90.0F, 6.0F},
                                                                {93.0F, 7.0F},
                                                                {101.0F, 11.0F},
                                                                {104.0F, 14.0F},
                                                                {104.0F, 46.0F},
                                                                {21.0F, 46.0F},
                                                                {19.0F, 45.0F},
                                                                {11.0F, 39.0F},
                                                                {1.0F, 29.0F},
                                                                {0.0F, 27.0F}}};
        static const std::vector<std::array<float, 2>> botHull{{{0.0F, 35.0F},
                                                                {1.0F, 33.0F},
                                                                {6.0F, 26.0F},
                                                                {8.0F, 24.0F},
                                                                {16.0F, 18.0F},
                                                                {18.0F, 17.0F},
                                                                {71.0F, 0.0F},
                                                                {80.0F, 0.0F},
                                                                {83.0F, 1.0F},
                                                                {119.0F, 17.0F},
                                                                {125.0F, 21.0F},
                                                                {138.0F, 30.0F},
                                                                {143.0F, 34.0F},
                                                                {145.0F, 39.0F},
                                                                {0.0F, 39.0F}}};
        const auto& hull = typeId == 9 ? topHull : (typeId == 10 ? midHull : botHull);
        registry.emplace<ColliderComponent>(id, ColliderComponent::polygon(hull));
    }
    if (data->frameCount > 1) {
        auto anim = AnimationComponent::create(data->frameCount, data->frameDuration);
        if (data->animation != nullptr) {
            const auto* clip = reinterpret_cast<const AnimationClip*>(data->animation);
            anim.frameIndices.clear();
            for (std::uint32_t i = 0; i < clip->frames.size(); ++i) {
                anim.frameIndices.push_back(i);
            }
            anim.frameTime = clip->frameTime;
            anim.loop      = clip->loop;
        }
        registry.emplace<AnimationComponent>(id, anim);
    }
}

void ReplicationSystem::applyEntity(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    applyTransform(registry, id, entity);
    applyVelocity(registry, id, entity);
    applyHealth(registry, id, entity);
    applyLives(registry, id, entity);
    applyScore(registry, id, entity);
    applyStatus(registry, id, entity);
    applyDead(registry, id, entity);
}

void ReplicationSystem::applyTransform(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    if (!entity.posX.has_value() && !entity.posY.has_value()) {
        return;
    }
    auto* comp = registry.has<TransformComponent>(id) ? &registry.get<TransformComponent>(id) : nullptr;
    if (comp == nullptr) {
        TransformComponent t{};
        if (entity.entityType.has_value()) {
            auto [sx, sy] = defaultScaleForType(*entity.entityType);
            t.scaleX      = sx;
            t.scaleY      = sy;
        }
        if (entity.posX.has_value())
            t.x = *entity.posX;
        if (entity.posY.has_value())
            t.y = *entity.posY;
        registry.emplace<TransformComponent>(id, t);
        return;
    }
    if (entity.posX.has_value())
        comp->x = *entity.posX;
    if (entity.posY.has_value())
        comp->y = *entity.posY;
}

void ReplicationSystem::applyVelocity(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    if (!entity.velX.has_value() && !entity.velY.has_value()) {
        return;
    }
    auto* comp = registry.has<VelocityComponent>(id) ? &registry.get<VelocityComponent>(id) : nullptr;
    if (comp == nullptr) {
        VelocityComponent v{};
        if (entity.velX.has_value())
            v.vx = *entity.velX;
        if (entity.velY.has_value())
            v.vy = *entity.velY;
        registry.emplace<VelocityComponent>(id, v);
        return;
    }
    if (entity.velX.has_value())
        comp->vx = *entity.velX;
    if (entity.velY.has_value())
        comp->vy = *entity.velY;
}

void ReplicationSystem::applyHealth(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    if (!entity.health.has_value()) {
        return;
    }
    auto* comp = registry.has<HealthComponent>(id) ? &registry.get<HealthComponent>(id) : nullptr;
    if (comp == nullptr) {
        registry.emplace<HealthComponent>(id, HealthComponent::create(*entity.health));
        return;
    }
    comp->current = *entity.health;
    if (*entity.health > comp->max) {
        comp->max = *entity.health;
    }
}

void ReplicationSystem::applyLives(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    if (entity.lives.has_value()) {
        if (!registry.has<LivesComponent>(id)) {
            registry.emplace<LivesComponent>(id, LivesComponent::create(*entity.lives, 3));
        }
        auto& lives   = registry.get<LivesComponent>(id);
        int previous  = lives.current;
        lives.current = *entity.lives;

        auto itPrev = lastKnownLives_.find(entity.entityId);
        if (itPrev != lastKnownLives_.end()) {
            previous = itPrev->second;
        }
        if (*entity.lives < previous) {
            respawnCooldown_[entity.entityId] = 2.0F;
        }
        lastKnownLives_[entity.entityId] = *entity.lives;
    }
}

void ReplicationSystem::applyScore(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    if (!entity.score.has_value()) {
        return;
    }
    if (!registry.has<ScoreComponent>(id)) {
        registry.emplace<ScoreComponent>(id, ScoreComponent::create(*entity.score));
        return;
    }
    auto& score = registry.get<ScoreComponent>(id);
    score.set(*entity.score);
}

void ReplicationSystem::applyStatusEffects(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    if (!entity.statusEffects.has_value()) {
        return;
    }
    std::uint8_t status = *entity.statusEffects;
    bool isInvincible   = (status & (1 << 1)) != 0;

    if (isInvincible) {
        if (!registry.has<InvincibilityComponent>(id)) {
            registry.emplace<InvincibilityComponent>(id, InvincibilityComponent::create(2.0F));
        }
    } else {
        if (registry.has<InvincibilityComponent>(id)) {
            registry.remove<InvincibilityComponent>(id);
        }
        respawnCooldown_.erase(entity.entityId);
    }
}
void ReplicationSystem::applyStatus(Registry&, EntityId, const SnapshotEntity&) {}

void ReplicationSystem::applyDead(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    if (!entity.dead.has_value()) {
        return;
    }
    if (*entity.dead && registry.isAlive(id)) {
        registry.destroyEntity(id);
        for (auto it = remoteToLocal_.begin(); it != remoteToLocal_.end(); ++it) {
            if (it->second == id) {
                remoteToLocal_.erase(it);
                break;
            }
        }
    }
}

void ReplicationSystem::applyInterpolation(Registry& registry, EntityId id, const SnapshotEntity& entity, std::uint32_t)
{
    if (!entity.posX.has_value() && !entity.posY.has_value()) {
        return;
    }
    auto* interp = registry.has<InterpolationComponent>(id) ? &registry.get<InterpolationComponent>(id) : nullptr;
    if (interp == nullptr) {
        InterpolationComponent ic{};
        ic.previousX = entity.posX.value_or(0.0F);
        ic.previousY = entity.posY.value_or(0.0F);
        ic.targetX   = entity.posX.value_or(ic.previousX);
        ic.targetY   = entity.posY.value_or(ic.previousY);
        ic.velocityX = entity.velX.value_or(0.0F);
        ic.velocityY = entity.velY.value_or(0.0F);
        registry.emplace<InterpolationComponent>(id, ic);
        return;
    }
    float prevX = interp->targetX;
    float prevY = interp->targetY;
    float newX  = entity.posX.value_or(prevX);
    float newY  = entity.posY.value_or(prevY);

    float dx = newX - prevX;
    float dy = newY - prevY;
    if (dx * dx + dy * dy > 10000.0F) {
        interp->previousX = newX;
        interp->previousY = newY;
    } else {
        interp->previousX = prevX;
        interp->previousY = prevY;
    }

    interp->targetX     = newX;
    interp->targetY     = newY;
    interp->elapsedTime = 0.0F;
    if (entity.velX.has_value())
        interp->velocityX = *entity.velX;
    if (entity.velY.has_value())
        interp->velocityY = *entity.velY;
}
