#include "server/ServerRunner.hpp"

#include "Logger.hpp"
#include "config/EntityTypeIds.hpp"
#include "config/WorldConfig.hpp"
#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"

#include <algorithm>
#include <chrono>
#include <random>
#include <thread>

namespace
{
    constexpr double kTickRate = 60.0;

    std::uint8_t typeForEntity(const Registry& registry, EntityId id)
    {
        if (registry.has<TypeComponent>(id)) {
            return static_cast<std::uint8_t>(registry.get<TypeComponent>(id).typeId);
        }
        if (registry.has<TagComponent>(id)) {
            const auto& tag = registry.get<TagComponent>(id);
            if (tag.hasTag(EntityTag::Player))
                return static_cast<std::uint8_t>(toTypeId(EntityTypeId::Player));
            if (tag.hasTag(EntityTag::Projectile))
                return static_cast<std::uint8_t>(toTypeId(EntityTypeId::Projectile));
            if (tag.hasTag(EntityTag::Obstacle))
                return static_cast<std::uint8_t>(toTypeId(EntityTypeId::ObstacleSmall));
            if (tag.hasTag(EntityTag::Enemy))
                return static_cast<std::uint8_t>(toTypeId(EntityTypeId::Enemy));
        }
        return static_cast<std::uint8_t>(toTypeId(EntityTypeId::Enemy));
    }
} // namespace

ServerApp::ServerApp(std::uint16_t port, std::atomic<bool>& runningFlag)
    : playerInputSys_(250.0F, 500.0F, 2.0F, 10), movementSys_(),
      monsterSpawnSys_(MonsterSpawnConfig{.spawnInterval = 2.0F, .spawnX = 1200.0F, .yMin = 300.0F, .yMax = 500.0F},
                       {MovementComponent::linear(150.0F), MovementComponent::sine(150.0F, 100.0F, 0.5F),
                        MovementComponent::zigzag(150.0F, 80.0F, 1.0F)},
                       static_cast<std::uint32_t>(std::chrono::system_clock::now().time_since_epoch().count())),
      obstacleSpawnSys_(ObstacleSpawnConfig{},
                        {ObstacleVariant{toTypeId(EntityTypeId::ObstacleSmall),
                                         toTypeId(EntityTypeId::ObstacleTopSmall), "obstacle_mountain_small",
                                         "obstacle_mountain_small_top", 220.0F, 170.0F, 220.0F, 108.0F, 62.0F, 0.0F},
                         ObstacleVariant{toTypeId(EntityTypeId::ObstacleMedium),
                                         toTypeId(EntityTypeId::ObstacleTopMedium), "obstacle_mountain_medium",
                                         "obstacle_mountain_medium_top", 280.0F, 210.0F, 280.0F, 138.0F, 72.0F, 0.0F},
                         ObstacleVariant{toTypeId(EntityTypeId::ObstacleLarge),
                                         toTypeId(EntityTypeId::ObstacleTopLarge), "obstacle_mountain_large",
                                         "obstacle_mountain_large_top", 360.0F, 260.0F, 360.0F, 171.0F, 89.0F, 0.0F}}),
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
    handleControl();
    maybeStartGame();
    updateCountdown(1.0F / kTickRate);
    if (!gameStarted_) {
        return;
    }
    auto mapped = mapInputs(inputs);
    playerInputSys_.update(registry_, mapped);
    obstacleSpawnSys_.update(registry_, 1.0F / kTickRate);
    movementSys_.update(registry_, 1.0F / kTickRate);
    monsterMovementSys_.update(registry_, 1.0F / kTickRate);
    monsterSpawnSys_.update(registry_, 1.0F / kTickRate);
    enemyShootingSys_.update(registry_, 1.0F / kTickRate);

    cleanupOffscreenEntities();

    auto collisions = collisionSys_.detect(registry_);
    logCollisions(collisions);
    damageSys_.apply(registry_, collisions);

    std::vector<EntityId> toDestroy;
    for (EntityId id : registry_.view<HealthComponent>()) {
        if (registry_.isAlive(id) && registry_.get<HealthComponent>(id).current <= 0) {
            toDestroy.push_back(id);
        }
    }
    if (!toDestroy.empty()) {
        Logger::instance().info("Destroying " + std::to_string(toDestroy.size()) + " dead entity(ies)");
        for (EntityId id : toDestroy) {
            EntityDestroyedPacket pkt{};
            pkt.entityId = id;
            sendThread_.broadcast(pkt);
        }
    }
    destructionSys_.update(registry_, toDestroy);
    std::unordered_set<EntityId> current;
    for (EntityId id : registry_.view<TransformComponent>()) {
        if (registry_.isAlive(id)) {
            current.insert(id);
            if (!knownEntities_.contains(id)) {
                EntitySpawnPacket pkt{};
                pkt.entityId   = id;
                pkt.entityType = typeForEntity(registry_, id);
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

    auto snapshots = buildSnapshotChunks(registry_, currentTick_, 1000);
    for (const auto& pkt : snapshots) {
        for (const auto& c : clients_) {
            sendThread_.sendTo(pkt, c);
        }
    }
    currentTick_++;
}

void ServerApp::cleanupOffscreenEntities()
{
    std::vector<EntityId> offscreenEntities;
    const float leftBound =
        std::min(-150.0F, obstacleSpawnSys_.cleanupLeftBound()); // allow larger sprites to exit fully
    const float rightBound = std::max(WorldConfig::Width + 600.0F, obstacleSpawnSys_.cleanupRightBound());

    for (EntityId id : registry_.view<TransformComponent, TagComponent>()) {
        if (!registry_.isAlive(id)) {
            continue;
        }
        auto& t   = registry_.get<TransformComponent>(id);
        auto& tag = registry_.get<TagComponent>(id);
        bool removableTag =
            tag.hasTag(EntityTag::Enemy) || tag.hasTag(EntityTag::Projectile) || tag.hasTag(EntityTag::Obstacle);
        bool outOfBounds = t.x < leftBound || t.x > rightBound;
        if (removableTag && outOfBounds) {
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
    if (tag.hasTag(EntityTag::Projectile)) {
        return "Projectile";
    }
    if (tag.hasTag(EntityTag::Obstacle)) {
        return "Obstacle";
    }
    if (tag.hasTag(EntityTag::Background)) {
        return "Background";
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
    levelSeed_   = 0;
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
}
