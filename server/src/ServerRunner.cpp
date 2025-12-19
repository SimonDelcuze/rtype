#include "server/ServerRunner.hpp"

#include "Logger.hpp"
#include "components/InvincibilityComponent.hpp"
#include "components/LivesComponent.hpp"
#include "components/RespawnTimerComponent.hpp"
#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"
#include "server/EntityTypeResolver.hpp"
#include "server/SpawnConfig.hpp"

#include <algorithm>
#include <chrono>
#include <random>
#include <thread>

ServerApp::ServerApp(std::uint16_t port, std::atomic<bool>& runningFlag)
    : levelScript_(buildSpawnSetupForLevel(1)), playerInputSys_(250.0F, 500.0F, 2.0F, 10), movementSys_(),
      monsterSpawnSys_(levelScript_.patterns, levelScript_.spawns), obstacleSpawnSys_(levelScript_.obstacles),
      monsterMovementSys_(), enemyShootingSys_(), damageSys_(eventBus_), destructionSys_(eventBus_),
      receiveThread_(IpEndpoint{.addr = {0, 0, 0, 0}, .port = port}, inputQueue_, controlQueue_, &timeoutQueue_,
                     std::chrono::seconds(30)),
      sendThread_(IpEndpoint{.addr = {0, 0, 0, 0}, .port = 0}, clients_, kTickRate),
      gameLoop_(
          inputQueue_, [this](const std::vector<ReceivedInput>& inputs) { tick(inputs); }, kTickRate),
      running_(&runningFlag)
{}

bool ServerApp::start()
{
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
    monsterSpawnSys_.update(registry_, deltaTime);
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
    monsterSpawnSys_.reset();
    obstacleSpawnSys_.reset();
}

void ServerApp::updateRespawnTimers(float deltaTime)
{
    std::vector<EntityId> respawned;
    for (EntityId id : registry_.view<RespawnTimerComponent>()) {
        auto& timer = registry_.get<RespawnTimerComponent>(id);
        timer.timeLeft -= deltaTime;
        if (timer.timeLeft <= 0.0F) {
            respawned.push_back(id);
        }
    }
    for (EntityId id : respawned) {
        registry_.remove<RespawnTimerComponent>(id);
        if (registry_.has<HealthComponent>(id)) {
            auto& h   = registry_.get<HealthComponent>(id);
            h.current = h.max;
        }
        if (registry_.has<TransformComponent>(id)) {
            auto& t = registry_.get<TransformComponent>(id);
            t.x     = 200.0F;
            t.y     = 300.0F;
        }
        registry_.emplace<InvincibilityComponent>(id, InvincibilityComponent::create(3.0F));
        Logger::instance().info("Player (ID:" + std::to_string(id) + ") respawned. Y reset to 300.");
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

void ServerApp::handleDeathAndRespawn()
{
    std::vector<EntityId> toDestroy;
    for (EntityId id : registry_.view<HealthComponent>()) {
        if (!registry_.isAlive(id))
            continue;
        auto& health = registry_.get<HealthComponent>(id);
        if (health.current <= 0) {
            if (registry_.has<LivesComponent>(id)) {
                auto& lives = registry_.get<LivesComponent>(id);
                if (lives.current > 0) {
                    if (!registry_.has<RespawnTimerComponent>(id)) {
                        lives.loseLife();
                        registry_.emplace<RespawnTimerComponent>(id, RespawnTimerComponent::create(2.0F));
                        if (registry_.has<TransformComponent>(id)) {
                            registry_.get<TransformComponent>(id).y = -10000.0F;
                        }
                    }
                    continue;
                }
            }
            toDestroy.push_back(id);
        }
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
