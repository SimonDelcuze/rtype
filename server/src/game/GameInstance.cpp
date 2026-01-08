#include "game/GameInstance.hpp"

#include "Logger.hpp"
#include "components/InvincibilityComponent.hpp"
#include "components/LivesComponent.hpp"
#include "components/RespawnTimerComponent.hpp"
#include "core/EntityTypeResolver.hpp"
#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"
#include "network/LevelEventData.hpp"
#include "network/ServerBroadcastPacket.hpp"
#include "network/ServerDisconnectPacket.hpp"

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

    LevelScrollSettings toNetworkScroll(const ScrollSettings& scroll)
    {
        LevelScrollSettings out;
        out.mode   = static_cast<LevelScrollMode>(scroll.mode);
        out.speedX = scroll.speedX;
        out.curve.reserve(scroll.curve.size());
        for (const auto& key : scroll.curve) {
            out.curve.push_back(LevelScrollKeyframe{key.time, key.speedX});
        }
        return out;
    }

    LevelCameraBounds toNetworkCamera(const CameraBounds& bounds)
    {
        LevelCameraBounds out;
        out.minX = bounds.minX;
        out.maxX = bounds.maxX;
        out.minY = bounds.minY;
        out.maxY = bounds.maxY;
        return out;
    }

    std::optional<LevelEventData> toLevelEventData(const LevelEvent& event)
    {
        LevelEventData data;
        if (event.type == EventType::SetScroll) {
            if (!event.scroll.has_value())
                return std::nullopt;
            data.type   = LevelEventType::SetScroll;
            data.scroll = toNetworkScroll(*event.scroll);
            return data;
        }
        if (event.type == EventType::SetBackground) {
            if (!event.backgroundId.has_value())
                return std::nullopt;
            data.type         = LevelEventType::SetBackground;
            data.backgroundId = *event.backgroundId;
            return data;
        }
        if (event.type == EventType::SetMusic) {
            if (!event.musicId.has_value())
                return std::nullopt;
            data.type    = LevelEventType::SetMusic;
            data.musicId = *event.musicId;
            return data;
        }
        if (event.type == EventType::SetCameraBounds) {
            if (!event.cameraBounds.has_value())
                return std::nullopt;
            data.type         = LevelEventType::SetCameraBounds;
            data.cameraBounds = toNetworkCamera(*event.cameraBounds);
            return data;
        }
        if (event.type == EventType::GateOpen) {
            if (!event.gateId.has_value())
                return std::nullopt;
            data.type   = LevelEventType::GateOpen;
            data.gateId = *event.gateId;
            return data;
        }
        if (event.type == EventType::GateClose) {
            if (!event.gateId.has_value())
                return std::nullopt;
            data.type   = LevelEventType::GateClose;
            data.gateId = *event.gateId;
            return data;
        }
        return std::nullopt;
    }
} // namespace

GameInstance::GameInstance(std::uint32_t roomId, std::uint16_t port, std::atomic<bool>& runningFlag)
    : roomId_(roomId), port_(port), world_(), registry_(world_.getRegistry()),
      playerInputSys_(250.0F, 500.0F, 2.0F, 10), movementSys_(), monsterMovementSys_(), enemyShootingSys_(),
      damageSys_(eventBus_), scoreSys_(eventBus_, registry_), destructionSys_(eventBus_),
      receiveThread_(IpEndpoint{.addr = {0, 0, 0, 0}, .port = port}, inputQueue_, controlQueue_, &timeoutQueue_,
                     std::chrono::seconds(30)),
      sendThread_(IpEndpoint{.addr = {0, 0, 0, 0}, .port = 0}, clients_, kTickRate, roomId),
      gameLoop_(
          inputQueue_, [this](const std::vector<ReceivedInput>& inputs) { tick(inputs); }, kTickRate),
      running_(&runningFlag), networkBridge_(sendThread_)
{
    LevelLoadError error;
    if (LevelLoader::load(1, levelData_, error)) {
        levelLoaded_   = true;
        levelDirector_ = std::make_unique<LevelDirector>(levelData_);
        levelSpawnSys_ = std::make_unique<LevelSpawnSystem>(levelData_, levelDirector_.get());

        world_.setLevelLoaded(true);
        world_.setLevelDirector(std::make_unique<LevelDirector>(levelData_));
        world_.setLevelSpawnSystem(std::make_unique<LevelSpawnSystem>(levelData_, world_.getLevelDirector()));
    } else {
        levelLoaded_ = false;
        world_.setLevelLoaded(false);
        world_.setLevelLoaded(false);
        logError("[Level] Level load failed: " + error.message + " path=" + error.path + " ptr=" + error.jsonPointer);
    }
}

bool GameInstance::start()
{
    if (!levelLoaded_) {
        logError("[Level] Server start aborted: level not loaded");
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

void GameInstance::run()
{
    while (running_ && running_->load()) {
        processTimeouts();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void GameInstance::stop(const std::string& reason)
{
    notifyDisconnection(reason);
    gameLoop_.stop();
    sendThread_.stop();
    receiveThread_.stop();
}

void GameInstance::notifyDisconnection(const std::string& reason)
{
    auto pkt   = ServerDisconnectPacket::create(reason);
    auto bytes = pkt.encode();
    std::vector<std::uint8_t> vec(bytes.begin(), bytes.end());
    for (const auto& client : clients_) {
        sendThread_.sendTo(vec, client);
    }
}

void GameInstance::broadcast(const std::string& message)
{
    auto pkt   = ServerBroadcastPacket::create(message);
    auto bytes = pkt.encode();
    std::vector<std::uint8_t> vec(bytes.begin(), bytes.end());
    for (const auto& client : clients_) {
        sendThread_.sendTo(vec, client);
    }
}

std::size_t GameInstance::getPlayerCount() const
{
    return sessions_.size();
}

std::vector<ClientSession> GameInstance::getSessions() const
{
    std::vector<ClientSession> sessions;
    sessions.reserve(sessions_.size());
    for (const auto& [key, session] : sessions_) {
        sessions.push_back(session);
    }
    return sessions;
}

void GameInstance::kickPlayer(std::uint32_t playerId)
{
    std::string keyToRemove;
    bool found = false;
    IpEndpoint endpoint;

    for (const auto& [key, session] : sessions_) {
        if (session.playerId == playerId) {
            keyToRemove = key;
            endpoint    = session.endpoint;
            found       = true;
            break;
        }
    }

    if (found) {
        auto pkt   = ServerDisconnectPacket::create("You were kicked from the room");
        auto bytes = pkt.encode();
        std::vector<std::uint8_t> vec(bytes.begin(), bytes.end());
        sendThread_.sendTo(vec, endpoint);

        logInfo("[Admin] Kicked player " + std::to_string(playerId) + " (" + endpointKey(endpoint) + ")");
        onDisconnect(endpoint);
    } else {
        logWarn("[Admin] Kick failed: player " + std::to_string(playerId) + " not found");
    }
}

bool GameInstance::isEmpty() const
{
    return sessions_.empty();
}

void GameInstance::handleInput(const ReceivedInput& input)
{
    inputQueue_.push(input);
}

void GameInstance::handleControlEvent(const ControlEvent& ctrl)
{
    controlQueue_.push(ctrl);
}

void GameInstance::handleTimeout(const ClientTimeoutEvent& timeout)
{
    timeoutQueue_.push(timeout);
}

void GameInstance::resetGame()

{
    logInfo("[Game] Resetting game state...");
    registry_.clear();
    playerEntities_.clear();
    sessions_.clear();
    clients_.clear();
    sendThread_.setClients(clients_);
    sendThread_.clearLatest();
    currentTick_ = 0;
    gameStarted_ = false;
    introCinematic_.reset();
    eventBus_.clear();
    networkBridge_.clear();
    ControlEvent ctrl;
    while (controlQueue_.tryPop(ctrl))
        ;
    ReceivedInput input;
    while (inputQueue_.tryPop(input))
        ;

    ClientTimeoutEvent timeout;
    while (timeoutQueue_.tryPop(timeout))
        ;
    logInfo("[Game] Game state reset complete");
    if (levelLoaded_) {
        levelDirector_->reset();
        levelSpawnSys_->reset();
    }
    checkpointState_.reset();
    lastSegmentIndex_ = -1;
}

void GameInstance::updateRespawnTimers(float deltaTime)
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

void GameInstance::updateInvincibilityTimers(float deltaTime)
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
        logInfo("[Player] Player (ID:" + std::to_string(id) + ") is no longer invincible.");
    }
}

void GameInstance::captureCheckpoint(const std::vector<DispatchedEvent>& events)
{
    if (!levelLoaded_) {
        return;
    }
    for (const auto& dispatched : events) {
        const auto& event = dispatched.event;
        if (event.type != EventType::Checkpoint || !event.checkpoint.has_value())
            continue;
        CheckpointState state;
        state.director   = levelDirector_->captureCheckpointState();
        state.spawns     = levelSpawnSys_->captureCheckpointState();
        state.respawn    = event.checkpoint->respawn;
        checkpointState_ = std::move(state);
    }
}

void GameInstance::sendLevelEvents(const std::vector<DispatchedEvent>& events)
{
    if (!levelLoaded_) {
        return;
    }
    for (const auto& dispatched : events) {
        auto data = toLevelEventData(dispatched.event);
        if (!data.has_value())
            continue;
        auto pkt = buildLevelEventPacket(*data, currentTick_);
        if (pkt.empty())
            continue;
        for (const auto& c : clients_) {
            sendThread_.sendTo(pkt, c);
        }
    }
}

void GameInstance::sendSegmentState()
{
    if (!levelLoaded_) {
        return;
    }
    std::int32_t current = levelDirector_->currentSegmentIndex();
    if (current < 0 || current == lastSegmentIndex_) {
        return;
    }
    lastSegmentIndex_ = current;
    const auto* seg   = levelDirector_->currentSegment();
    if (seg == nullptr) {
        return;
    }
    LevelEventData scrollEvent;
    scrollEvent.type   = LevelEventType::SetScroll;
    scrollEvent.scroll = toNetworkScroll(seg->scroll);
    auto scrollPkt     = buildLevelEventPacket(scrollEvent, currentTick_);
    if (!scrollPkt.empty()) {
        for (const auto& c : clients_) {
            sendThread_.sendTo(scrollPkt, c);
        }
    }
    if (seg->cameraBounds.has_value()) {
        LevelEventData camEvent;
        camEvent.type         = LevelEventType::SetCameraBounds;
        camEvent.cameraBounds = toNetworkCamera(*seg->cameraBounds);
        auto camPkt           = buildLevelEventPacket(camEvent, currentTick_);
        if (!camPkt.empty()) {
            for (const auto& c : clients_) {
                sendThread_.sendTo(camPkt, c);
            }
        }
    }
}

void GameInstance::purgeNonPlayerEntities()
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

void GameInstance::respawnPlayers(const Vec2f& respawn)
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

void GameInstance::resetToCheckpoint()
{
    Vec2f respawn = kDefaultRespawn;
    std::vector<LevelDirector::BossCheckpointState> bossStates;

    if (levelLoaded_) {
        if (checkpointState_.has_value()) {
            levelDirector_->restoreCheckpointState(checkpointState_->director);
            levelSpawnSys_->restoreCheckpointState(checkpointState_->spawns);
            respawn    = checkpointState_->respawn;
            bossStates = checkpointState_->director.bosses;
        } else {
            levelDirector_->reset();
            levelSpawnSys_->reset();
        }
    }
    lastSegmentIndex_ = -1;

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

void GameInstance::spawnPlayerDeathFx(float x, float y)
{
    EntityId fx = registry_.createEntity();
    registry_.emplace<TransformComponent>(fx, TransformComponent::create(x, y));
    registry_.emplace<RenderTypeComponent>(fx, RenderTypeComponent::create(kPlayerDeathFxType));

    MissileComponent lifetime{};
    lifetime.damage   = 0;
    lifetime.lifetime = kPlayerDeathFxLifetime;
    registry_.emplace<MissileComponent>(fx, lifetime);
}

void GameInstance::handleDeathAndRespawn()
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

void GameInstance::logInfo(const std::string& msg) const
{
    Logger::instance().logToRoom(roomId_, "INFO", msg);
}

void GameInstance::logWarn(const std::string& msg) const
{
    Logger::instance().logToRoom(roomId_, "WARN", msg);
}

void GameInstance::logError(const std::string& msg) const
{
    Logger::instance().logToRoom(roomId_, "ERROR", msg);
}
