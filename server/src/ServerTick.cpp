#include "Logger.hpp"
#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"
#include "network/PacketHeader.hpp"
#include "server/EntityTypeResolver.hpp"
#include "server/ServerRunner.hpp"

#include <algorithm>

void ServerApp::tick(const std::vector<ReceivedInput>& inputs)
{
    constexpr float dt = 1.0F / kTickRate;

    handleControl();
    maybeStartGame();
    updateCountdown(dt);
    if (!gameStarted_) {
        return;
    }
    updateSystems(dt, inputs);

    auto collisions = collisionSys_.detect(registry_);
    logCollisions(collisions);
    damageSys_.apply(registry_, collisions);

    std::vector<EntityId> toDestroy = collectDeadEntities();
    broadcastDestructions(toDestroy);
    destructionSys_.update(registry_, toDestroy);

    auto current = collectCurrentEntities();
    syncEntityLifecycle(current);

    sendSnapshots();
    currentTick_++;
}

void ServerApp::updateSystems(float deltaTime, const std::vector<ReceivedInput>& inputs)
{
    auto mapped = mapInputs(inputs);
    playerInputSys_.update(registry_, mapped);
    movementSys_.update(registry_, deltaTime);
    boundarySys_.update(registry_);
    monsterMovementSys_.update(registry_, deltaTime);
    if (levelLoaded_) {
        levelDirector_->update(registry_, deltaTime);
        auto events = levelDirector_->consumeEvents();
        levelSpawnSys_->update(registry_, deltaTime, events);
    }
    enemyShootingSys_.update(registry_, deltaTime);

    cleanupExpiredMissiles(deltaTime);
    cleanupOffscreenEntities();
}

std::vector<EntityId> ServerApp::collectDeadEntities()
{
    std::vector<EntityId> toDestroy;
    for (EntityId id : registry_.view<HealthComponent>()) {
        if (registry_.isAlive(id) && registry_.get<HealthComponent>(id).current <= 0) {
            toDestroy.push_back(id);
        }
    }
    return toDestroy;
}

void ServerApp::broadcastDestructions(const std::vector<EntityId>& toDestroy)
{
    if (toDestroy.empty()) {
        return;
    }
    Logger::instance().info("Destroying " + std::to_string(toDestroy.size()) + " dead entity(ies)");
    for (EntityId id : toDestroy) {
        EntityDestroyedPacket pkt{};
        pkt.entityId = id;
        sendThread_.broadcast(pkt);
    }
}

std::unordered_set<EntityId> ServerApp::collectCurrentEntities()
{
    std::unordered_set<EntityId> current;
    for (EntityId id : registry_.view<TransformComponent>()) {
        if (registry_.isAlive(id)) {
            current.insert(id);
        }
    }
    return current;
}

void ServerApp::syncEntityLifecycle(const std::unordered_set<EntityId>& current)
{
    for (EntityId id : current) {
        if (!knownEntities_.contains(id)) {
            EntitySpawnPacket pkt{};
            pkt.entityId   = id;
            pkt.entityType = resolveEntityType(registry_, id);
            auto& t        = registry_.get<TransformComponent>(id);
            pkt.posX       = t.x;
            pkt.posY       = t.y;
            sendThread_.broadcast(pkt);
        }
    }

    for (EntityId oldId : knownEntities_) {
        if (!current.contains(oldId)) {
            EntityDestroyedPacket pkt{};
            pkt.entityId = oldId;
            sendThread_.broadcast(pkt);
        }
    }

    knownEntities_ = current;
}

void ServerApp::sendSnapshots()
{
    auto snapshotPkt                           = buildSnapshotPacket(registry_, currentTick_);
    constexpr std::size_t kSnapshotPayloadWarn = 1400;
    std::size_t payloadSize                    = 0;
    if (snapshotPkt.size() >= PacketHeader::kSize + PacketHeader::kCrcSize) {
        payloadSize = snapshotPkt.size() - PacketHeader::kSize - PacketHeader::kCrcSize;
    }
    if (payloadSize > kSnapshotPayloadWarn) {
        auto chunks = buildSnapshotChunks(registry_, currentTick_);
        for (const auto& c : clients_) {
            for (const auto& chunk : chunks) {
                sendThread_.sendTo(chunk, c);
            }
        }
        return;
    }
    for (const auto& c : clients_) {
        sendThread_.sendTo(snapshotPkt, c);
    }
}

void ServerApp::cleanupExpiredMissiles(float deltaTime)
{
    std::vector<EntityId> expired;
    for (EntityId id : registry_.view<MissileComponent>()) {
        if (!registry_.isAlive(id)) {
            continue;
        }
        auto& missile = registry_.get<MissileComponent>(id);
        missile.lifetime -= deltaTime;
        if (missile.lifetime <= 0.0F) {
            expired.push_back(id);
        }
    }
    if (expired.empty()) {
        return;
    }
    Logger::instance().info("Cleaning up " + std::to_string(expired.size()) + " expired missile(s)");
    for (EntityId id : expired) {
        EntityDestroyedPacket pkt{};
        pkt.entityId = id;
        sendThread_.broadcast(pkt);
        registry_.destroyEntity(id);
    }
}

void ServerApp::cleanupOffscreenEntities()
{
    std::vector<EntityId> offscreenEntities;
    for (EntityId id : registry_.view<TransformComponent, TagComponent>()) {
        if (!registry_.isAlive(id)) {
            continue;
        }
        auto& t   = registry_.get<TransformComponent>(id);
        auto& tag = registry_.get<TagComponent>(id);
        if ((tag.hasTag(EntityTag::Enemy) || tag.hasTag(EntityTag::Projectile)) && (t.x < -100.0F || t.x > 2000.0F)) {
            offscreenEntities.push_back(id);
        }
    }
    if (!offscreenEntities.empty()) {
        Logger::instance().info("Cleaning up " + std::to_string(offscreenEntities.size()) + " offscreen entity(ies)");
        for (EntityId id : offscreenEntities) {
            EntityDestroyedPacket pkt{};
            pkt.entityId = id;
            sendThread_.broadcast(pkt);
            registry_.destroyEntity(id);
        }
    }
}

std::string ServerApp::getEntityTagName(EntityId id) const
{
    if (!registry_.has<TagComponent>(id)) {
        return "Unknown";
    }
    const auto& tag = registry_.get<TagComponent>(id);
    if (tag.hasTag(EntityTag::Player)) {
        return "Player";
    }
    if (tag.hasTag(EntityTag::Enemy)) {
        return "Enemy";
    }
    if (tag.hasTag(EntityTag::Obstacle)) {
        return "Obstacle";
    }
    if (tag.hasTag(EntityTag::Projectile)) {
        return "Projectile";
    }
    return "Unknown";
}

void ServerApp::logCollisions(const std::vector<Collision>& collisions)
{
    if (collisions.empty()) {
        return;
    }
    Logger::instance().info("Detected " + std::to_string(collisions.size()) + " collision(s)");
    for (const auto& col : collisions) {
        std::string aTag = getEntityTagName(col.a);
        std::string bTag = getEntityTagName(col.b);
        std::string msg  = "  Collision: ";
        msg += aTag;
        msg += " (ID:";
        msg += std::to_string(col.a);
        msg += ") <-> ";
        msg += bTag;
        msg += " (ID:";
        msg += std::to_string(col.b);
        msg += ")";
        Logger::instance().info(msg);
    }
}
