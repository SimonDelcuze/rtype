#include "server/ServerRunner.hpp"

#include "Logger.hpp"
#include "components/InvincibilityComponent.hpp"
#include "components/LivesComponent.hpp"
#include "components/RespawnTimerComponent.hpp"
#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"
#include "server/EntityTypeResolver.hpp"

#include <algorithm>
#include <chrono>
#include <random>
#include <thread>

namespace
{
    constexpr std::uint16_t kPlayerDeathFxType   = 16;
    constexpr float kPlayerDeathFxLifetime       = 0.9F;
    constexpr float kOffscreenRespawnPlaceholder = -10000.0F;
    constexpr float kRespawnInvincibility        = 3.0F;
    const Vec2f kDefaultRespawn{100.0F, 400.0F};
} // namespace

ServerApp::ServerApp(std::uint16_t port, std::atomic<bool>& runningFlag)
    : playerInputSys_(250.0F, 500.0F, 2.0F, 10), movementSys_(), monsterMovementSys_(), enemyShootingSys_(),
      damageSys_(eventBus_), destructionSys_(eventBus_),
      receiveThread_(IpEndpoint{.addr = {0, 0, 0, 0}, .port = port}, inputQueue_, controlQueue_, &timeoutQueue_,
                     std::chrono::seconds(30)),
      sendThread_(IpEndpoint{.addr = {0, 0, 0, 0}, .port = 0}, clients_, kTickRate),
      gameLoop_(
          inputQueue_, [this](const std::vector<ReceivedInput>& inputs) { tick(inputs); }, kTickRate),
      running_(&runningFlag)
{
    LevelLoadError error;
    if (LevelLoader::load(1, levelData_, error)) {
        levelLoaded_   = true;
        levelDirector_ = std::make_unique<LevelDirector>(levelData_);
        levelSpawnSys_ = std::make_unique<LevelSpawnSystem>(levelData_, levelDirector_.get());
    } else {
        levelLoaded_ = false;
        Logger::instance().error("Level load failed: " + error.message + " path=" + error.path +
                                 " ptr=" + error.jsonPointer);
    }
}

bool ServerApp::start()
{
    if (!levelLoaded_) {
        Logger::instance().error("Server start aborted: level not loaded");
        return false;
    }
    if (!receiveThread_.start()) {
        return false;
    }
    if (!sendThread_.start()) {
        receiveThread_.stop();
        return false;
    }
    if (!gameLoop_.start()) {
        sendThread_.stop();
        receiveThread_.stop();
        return false;
    }
    return true;
}

void ServerApp::run()
{
    while (running_ && running_->load()) {
        processTimeouts();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ServerApp::stop()
{
    gameLoop_.stop();
    sendThread_.stop();
    receiveThread_.stop();
}

void ServerApp::tick(const std::vector<ReceivedInput>& inputs)
{
    const float deltaTime = 1.0F / kTickRate;

    handleControl();
    maybeStartGame();
    updateCountdown(deltaTime);
    if (!gameStarted_) {
        return;
    }

    auto mapped = mapInputs(inputs);
    playerInputSys_.update(registry_, mapped);

    movementSys_.update(registry_, deltaTime);
    monsterMovementSys_.update(registry_, deltaTime);
    if (levelLoaded_) {
        levelDirector_->update(registry_, deltaTime);
        auto events = levelDirector_->consumeEvents();
        levelSpawnSys_->update(registry_, deltaTime, events);
        captureCheckpoint(events);
    }
    enemyShootingSys_.update(registry_, deltaTime);
    boundarySys_.update(registry_);

    updateRespawnTimers(deltaTime);
    updateInvincibilityTimers(deltaTime);

    cleanupExpiredMissiles(deltaTime);
    cleanupOffscreenEntities();

    auto collisions = collisionSys_.detect(registry_);
    logCollisions(collisions);
    damageSys_.apply(registry_, collisions);

    handleDeathAndRespawn();
    syncEntityLifecycle();

    auto snapshotPkt = buildSnapshotPacket(registry_, currentTick_);
    for (const auto& c : clients_) {
        sendThread_.sendTo(snapshotPkt, c);
    }
    currentTick_++;
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

void ServerApp::resetGame()
{
    Logger::instance().info("Resetting game state...");
    registry_.clear();
    playerEntities_.clear();
    sessions_.clear();
    clients_.clear();
    sendThread_.setClients(clients_);
    sendThread_.clearLatest();
    currentTick_ = 0;
    gameStarted_ = false;
    eventBus_.clear();
    knownEntities_.clear();
    ControlEvent ctrl;
    while (controlQueue_.tryPop(ctrl))
        ;
    ReceivedInput input;
    while (inputQueue_.tryPop(input))
        ;

    ClientTimeoutEvent timeout;
    while (timeoutQueue_.tryPop(timeout))
        ;
    Logger::instance().info("Game state reset complete");
    if (levelLoaded_) {
        levelDirector_->reset();
        levelSpawnSys_->reset();
    }
    checkpointState_.reset();
}

void ServerApp::updateRespawnTimers(float deltaTime)
{
    bool shouldReset = false;
    for (EntityId id : registry_.view<RespawnTimerComponent>()) {
        auto& timer = registry_.get<RespawnTimerComponent>(id);
        timer.timeLeft -= deltaTime;
        if (timer.timeLeft <= 0.0F) {
            shouldReset = true;
        }
    }
    if (shouldReset) {
        resetToCheckpoint();
    }
}

void ServerApp::updateInvincibilityTimers(float deltaTime)
{
    std::vector<EntityId> vulnerable;
    for (EntityId id : registry_.view<InvincibilityComponent>()) {
        auto& inv = registry_.get<InvincibilityComponent>(id);
        inv.timeLeft -= deltaTime;
        if (inv.timeLeft <= 0.0F) {
            vulnerable.push_back(id);
        }
    }
    for (EntityId id : vulnerable) {
        registry_.remove<InvincibilityComponent>(id);
        Logger::instance().info("Player (ID:" + std::to_string(id) + ") is no longer invincible.");
    }
}

void ServerApp::captureCheckpoint(const std::vector<DispatchedEvent>& events)
{
    if (!levelLoaded_) {
        return;
    }
    for (const auto& dispatched : events) {
        const auto& event = dispatched.event;
        if (event.type != EventType::Checkpoint || !event.checkpoint.has_value())
            continue;
        CheckpointState state;
        state.director = levelDirector_->captureCheckpointState();
        state.spawns   = levelSpawnSys_->captureCheckpointState();
        state.respawn  = event.checkpoint->respawn;
        checkpointState_ = std::move(state);
    }
}

void ServerApp::purgeNonPlayerEntities()
{
    std::unordered_set<EntityId> players;
    for (const auto& [_, entityId] : playerEntities_) {
        players.insert(entityId);
    }
    std::vector<EntityId> toDestroy;
    for (EntityId id : registry_.view<TransformComponent>()) {
        if (!registry_.isAlive(id))
            continue;
        if (players.contains(id))
            continue;
        toDestroy.push_back(id);
    }
    for (EntityId id : toDestroy) {
        registry_.destroyEntity(id);
    }
}

void ServerApp::respawnPlayers(const Vec2f& respawn)
{
    for (const auto& [_, entityId] : playerEntities_) {
        if (!registry_.isAlive(entityId))
            continue;
        registry_.remove<RespawnTimerComponent>(entityId);
        if (registry_.has<HealthComponent>(entityId)) {
            auto& h   = registry_.get<HealthComponent>(entityId);
            h.current = h.max;
        }
        if (registry_.has<TransformComponent>(entityId)) {
            auto& t = registry_.get<TransformComponent>(entityId);
            t.x     = respawn.x;
            t.y     = respawn.y;
        }
        if (registry_.has<VelocityComponent>(entityId)) {
            auto& v = registry_.get<VelocityComponent>(entityId);
            v.vx    = 0.0F;
            v.vy    = 0.0F;
        }
        registry_.emplace<InvincibilityComponent>(entityId, InvincibilityComponent::create(kRespawnInvincibility));
    }
}

void ServerApp::resetToCheckpoint()
{
    Vec2f respawn = kDefaultRespawn;
    std::vector<LevelDirector::BossCheckpointState> bossStates;

    if (levelLoaded_) {
        if (checkpointState_.has_value()) {
            levelDirector_->restoreCheckpointState(checkpointState_->director);
            levelSpawnSys_->restoreCheckpointState(checkpointState_->spawns);
            respawn   = checkpointState_->respawn;
            bossStates = checkpointState_->director.bosses;
        } else {
            levelDirector_->reset();
            levelSpawnSys_->reset();
        }
    }

    purgeNonPlayerEntities();

    if (levelLoaded_ && checkpointState_.has_value()) {
        for (const auto& bossState : bossStates) {
            if (bossState.status != LevelDirector::BossCheckpointStatus::Alive)
                continue;
            auto settings = levelSpawnSys_->getBossSpawnSettings(bossState.bossId);
            if (!settings.has_value())
                continue;
            levelSpawnSys_->spawnBossImmediate(registry_, *settings);
        }
    }

    respawnPlayers(respawn);
}

void ServerApp::spawnPlayerDeathFx(float x, float y)
{
    EntityId fx = registry_.createEntity();
    registry_.emplace<TransformComponent>(fx, TransformComponent::create(x, y));
    registry_.emplace<RenderTypeComponent>(fx, RenderTypeComponent::create(kPlayerDeathFxType));

    MissileComponent lifetime{};
    lifetime.damage   = 0;
    lifetime.lifetime = kPlayerDeathFxLifetime;
    registry_.emplace<MissileComponent>(fx, lifetime);
}

void ServerApp::handleDeathAndRespawn()
{
    std::vector<EntityId> toDestroy;
    std::vector<std::pair<float, float>> deathFxToSpawn;
    for (EntityId id : registry_.view<HealthComponent>()) {
        if (!registry_.isAlive(id))
            continue;
        auto& health = registry_.get<HealthComponent>(id);
        if (health.current <= 0) {
            const bool isPlayer =
                registry_.has<TagComponent>(id) && registry_.get<TagComponent>(id).hasTag(EntityTag::Player);
            float deathX      = 0.0F;
            float deathY      = 0.0F;
            bool hadTransform = false;
            if (registry_.has<TransformComponent>(id)) {
                auto& t      = registry_.get<TransformComponent>(id);
                deathX       = t.x;
                deathY       = t.y;
                hadTransform = true;
            }
            if (registry_.has<LivesComponent>(id)) {
                auto& lives = registry_.get<LivesComponent>(id);
                if (lives.current > 0) {
                    if (!registry_.has<RespawnTimerComponent>(id)) {
                        lives.loseLife();
                        if (isPlayer && hadTransform) {
                            deathFxToSpawn.emplace_back(deathX, deathY);
                        }
                        registry_.emplace<RespawnTimerComponent>(id, RespawnTimerComponent::create(2.0F));
                        if (registry_.has<TransformComponent>(id)) {
                            registry_.get<TransformComponent>(id).y = kOffscreenRespawnPlaceholder;
                        }
                    }
                    continue;
                }
            }
            if (isPlayer && hadTransform) {
                deathFxToSpawn.emplace_back(deathX, deathY);
            }
            toDestroy.push_back(id);
        }
    }
    for (const auto& pos : deathFxToSpawn) {
        spawnPlayerDeathFx(pos.first, pos.second);
    }
    if (!toDestroy.empty()) {
        for (EntityId id : toDestroy) {
            EntityDestroyedPacket pkt{};
            pkt.entityId = id;
            sendThread_.broadcast(pkt);
        }
    }
    destructionSys_.update(registry_, toDestroy);
}

void ServerApp::syncEntityLifecycle()
{
    std::unordered_set<EntityId> current;
    for (EntityId id : registry_.view<TransformComponent>()) {
        if (registry_.isAlive(id)) {
            current.insert(id);
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
    }
    for (EntityId oldId : knownEntities_) {
        if (!current.contains(oldId)) {
            EntityDestroyedPacket pkt{};
            pkt.entityId = oldId;
            sendThread_.broadcast(pkt);
        }
    }
    knownEntities_ = std::move(current);
}
