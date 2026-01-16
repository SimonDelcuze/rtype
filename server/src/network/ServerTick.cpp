#include "Logger.hpp"
#include "core/EntityTypeResolver.hpp"
#include "components/PlayerInputComponent.hpp"
#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"
#include "network/PacketHeader.hpp"
#include "network/ServerRunner.hpp"
#include "simulation/GameEvent.hpp"
#include "simulation/PlayerCommand.hpp"

#include <algorithm>

void ServerApp::updateNetworkStats(float dt)
{
    static float statsTimer = 0.0F;
    statsTimer += dt;
    if (statsTimer >= 5.0F) {
        statsTimer = 0.0F;
        Logger::instance().logNetworkStats();
    }
}

void ServerApp::updateGameplay(float dt, const std::vector<ReceivedInput>& inputs)
{
    updateSystems(dt, inputs);

    auto collisions = collisionSys_.detect(registry_);
    logCollisions(collisions);
    damageSys_.apply(registry_, collisions);

    handleDeathAndRespawn();

    world_.trackEntityLifecycle();
    auto events = world_.consumeEvents();
    networkBridge_.processEvents(events);
}

void ServerApp::tick(const std::vector<ReceivedInput>& inputs)
{
    constexpr float dt = 1.0F / kTickRate;

    updateNetworkStats(dt);
    handleControl();
    maybeStartGame();
    updateCountdown(dt);

    if (gameStarted_) {
        updateGameplay(dt, inputs);
        sendSnapshots();
    }
    currentTick_++;
}

void ServerApp::updateSystems(float deltaTime, const std::vector<ReceivedInput>& inputs)
{
    introCinematic_.update(registry_, playerEntities_, deltaTime);
    const bool introActive = introCinematic_.active();

    std::vector<PlayerCommand> commands;
    if (!introActive) {
        auto mapped = mapInputs(inputs);
        commands    = convertInputsToCommands(mapped);
        playerInputSys_.update(registry_, commands);
    }

    movementSys_.update(registry_, deltaTime);
    boundarySys_.update(registry_);
    monsterMovementSys_.update(registry_, deltaTime);

    if (levelLoaded_) {
        const auto* segment = levelDirector_->currentSegment();
        if (segment != nullptr && segment->exit.type == TriggerType::PlayersReady) {
            for (const auto& cmd : commands) {
                levelDirector_->registerPlayerInput(cmd.playerId, cmd.inputFlags);
            }
        }
        float levelDelta = introActive ? 0.0F : deltaTime;
        levelDirector_->update(registry_, levelDelta);
        auto events = levelDirector_->consumeEvents();
        levelSpawnSys_->update(registry_, levelDelta, events);
        sendLevelEvents(events);
        sendSegmentState();
        playerBoundsSys_.update(registry_, levelDirector_->playerBounds());
    } else {
        playerBoundsSys_.update(registry_, std::nullopt);
    }

    enemyShootingSys_.update(registry_, deltaTime);
    walkerShotSys_.update(registry_, deltaTime);

    updateRespawnTimers(deltaTime);
    updateInvincibilityTimers(deltaTime);

    cleanupExpiredMissiles(deltaTime);
    cleanupOffscreenEntities();
}

std::unordered_set<EntityId> ServerApp::collectCurrentEntities()
{
    std::unordered_set<EntityId> current;
    auto& reg = world_.getRegistry();
    for (EntityId id : reg.view<TransformComponent>()) {
        if (reg.isAlive(id)) {
            current.insert(id);
        }
    }
    return current;
}

void ServerApp::logSnapshotSummary(std::size_t totalBytes, std::size_t payloadSize, bool forceFull)
{
    auto& reg = world_.getRegistry();
    Logger::instance().info("[Snapshot] tick=" + std::to_string(currentTick_) + " size=" + std::to_string(totalBytes) +
                            " payload=" + std::to_string(payloadSize) +
                            " entities=" + std::to_string(reg.entityCount()) + (forceFull ? " (FULL)" : " (delta)"));
}

void ServerApp::sendSnapshots()
{
    bool forceFull = (currentTick_ % kFullStateInterval == 0);
    auto result    = replicationManager_.synchronize(world_.getRegistry(), currentTick_, forceFull);

    if (result.packets.empty())
        return;

    std::size_t totalSize = 0;
    for (const auto& p : result.packets)
        totalSize += p.size();

    if (result.packets.size() > 1) {
        Logger::instance().info("[Snapshot] tick=" + std::to_string(currentTick_) +
                                " chunks=" + std::to_string(result.packets.size()) +
                                " total_size=" + std::to_string(totalSize) + (result.wasFull ? " (FULL)" : " (delta)"));
    } else {
        logSnapshotSummary(result.packets[0].size(), 0, result.wasFull);
    }

    for (const auto& c : clients_) {
        std::uint16_t lastSeq = 0;
        std::string key = endpointKey(c);
        auto itSess = sessions_.find(key);
        if (itSess != sessions_.end()) {
            auto itEnt = playerEntities_.find(itSess->second.playerId);
            if (itEnt != playerEntities_.end() && registry_.isAlive(itEnt->second)) {
                if (registry_.has<PlayerInputComponent>(itEnt->second)) {
                    lastSeq = registry_.get<PlayerInputComponent>(itEnt->second).sequenceId;
                }
            }
        }

        for (auto p : result.packets) {
            if (p.size() >= PacketHeader::kSize) {
                p[7] = static_cast<std::uint8_t>((lastSeq >> 8) & 0xFF);
                p[8] = static_cast<std::uint8_t>(lastSeq & 0xFF);

                if (p.size() >= PacketHeader::kSize + 4) {
                    std::uint32_t newCrc = PacketHeader::crc32(p.data(), p.size() - 4);
                    p[p.size() - 4] = static_cast<std::uint8_t>((newCrc >> 24) & 0xFF);
                    p[p.size() - 3] = static_cast<std::uint8_t>((newCrc >> 16) & 0xFF);
                    p[p.size() - 2] = static_cast<std::uint8_t>((newCrc >> 8) & 0xFF);
                    p[p.size() - 1] = static_cast<std::uint8_t>(newCrc & 0xFF);
                }
            }
            sendThread_.sendTo(p, c);
        }
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
    Logger::instance().info("[Replication] Cleaning up " + std::to_string(expired.size()) + " expired missile(s)");
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
        Logger::instance().info("[Replication] Cleaning up " + std::to_string(offscreenEntities.size()) +
                                " offscreen entity(ies)");
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
    Logger::instance().info("[Collision] Detected " + std::to_string(collisions.size()) + " collision(s)");
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
        Logger::instance().info("[Collision] " + msg);
    }
}

std::vector<PlayerCommand> ServerApp::convertInputsToCommands(const std::vector<ReceivedInput>& inputs) const
{
    std::vector<PlayerCommand> commands;
    commands.reserve(inputs.size());

    for (const auto& input : inputs) {
        PlayerCommand cmd;
        cmd.playerId   = input.input.playerId;
        cmd.inputFlags = input.input.flags;
        cmd.x          = input.input.x;
        cmd.y          = input.input.y;
        cmd.angle      = input.input.angle;
        cmd.sequenceId = input.input.sequenceId;
        cmd.tickId     = input.input.tickId;
        commands.push_back(cmd);
    }

    return commands;
}
