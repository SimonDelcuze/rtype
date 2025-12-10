#include "server/ServerRunner.hpp"

#include "Logger.hpp"

#include <chrono>
#include <random>
#include <thread>

namespace
{
    constexpr double kTickRate = 60.0;
}

ServerApp::ServerApp(std::uint16_t port, std::atomic<bool>& runningFlag)
    : playerInputSys_(250.0F, 500.0F, 2.0F, 10), movementSys_(),
      monsterSpawnSys_(MonsterSpawnConfig{.spawnInterval = 2.0F, .spawnX = 1200.0F, .yMin = 300.0F, .yMax = 500.0F},
                       {MovementComponent::linear(150.0F), MovementComponent::sine(150.0F, 100.0F, 0.5F),
                        MovementComponent::zigzag(150.0F, 80.0F, 1.0F)},
                       static_cast<std::uint32_t>(std::chrono::system_clock::now().time_since_epoch().count())),
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
    }
    destructionSys_.update(registry_, toDestroy);

    auto snapshot = buildSnapshotPacket(registry_, currentTick_);
    if (!snapshot.empty()) {
        sendThread_.publish(snapshot);
    }
    currentTick_++;
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
        if (tag.hasTag(EntityTag::Enemy) && t.x < -100.0F) {
            offscreenEntities.push_back(id);
        }
    }
    if (!offscreenEntities.empty()) {
        Logger::instance().info("Cleaning up " + std::to_string(offscreenEntities.size()) + " offscreen entity(ies)");
        for (EntityId id : offscreenEntities) {
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
