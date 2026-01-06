#include "Logger.hpp"
#include "game/GameInstance.hpp"

#include <random>

void GameInstance::handleControl()
{
    ControlEvent ctrl{};
    while (controlQueue_.tryPop(ctrl)) {
        handleControlMessage(ctrl);
    }
}

void GameInstance::handleControlMessage(const ControlEvent& ctrl)
{
    auto key  = endpointKey(ctrl.from);
    auto type = ctrl.header.messageType;

    if (gameStarted_) {
        if (type == static_cast<std::uint8_t>(MessageType::ClientHello) ||
            type == static_cast<std::uint8_t>(MessageType::ClientJoinRequest)) {
            Logger::instance().warn("[Net] Rejecting connection - game already started");
            sendThread_.sendTo(buildJoinDeny(ctrl.header.sequenceId), ctrl.from);
            return;
        }
    }

    auto& sess    = sessions_[key];
    sess.endpoint = ctrl.from;
    if (sess.playerId == 0) {
        sess.playerId = nextPlayerId_++;
    }

    if (type == static_cast<std::uint8_t>(MessageType::ClientHello)) {
        sess.hello = true;
        sendThread_.sendTo(buildServerHello(ctrl.header.sequenceId), ctrl.from);
    } else if (type == static_cast<std::uint8_t>(MessageType::ClientJoinRequest)) {
        onJoin(sess, ctrl);
    } else if (type == static_cast<std::uint8_t>(MessageType::ClientReady)) {
        sess.ready = true;
    } else if (type == static_cast<std::uint8_t>(MessageType::ClientPing)) {
        sendThread_.sendTo(buildPong(ctrl.header), ctrl.from);
    } else if (type == static_cast<std::uint8_t>(MessageType::ClientDisconnect)) {
        onDisconnect(ctrl.from);
    }
}

void GameInstance::onJoin(ClientSession& sess, const ControlEvent& ctrl)
{
    if (gameStarted_) {
        Logger::instance().warn("[Net] Rejecting join request - game already started");
        sendThread_.sendTo(buildJoinDeny(ctrl.header.sequenceId), ctrl.from);
        return;
    }

    if (sess.playerId == 0) {
        sess.playerId = nextPlayerId_++;
    }

    sess.join = true;
    sendThread_.sendTo(buildJoinAccept(ctrl.header.sequenceId), ctrl.from);

    bool alreadyExists = false;
    for (const auto& ep : clients_) {
        if (endpointKey(ep) == endpointKey(ctrl.from)) {
            alreadyExists = true;
            break;
        }
    }
    if (!alreadyExists) {
        clients_.push_back(ctrl.from);
        sendThread_.setClients(clients_);
    }

    if (!playerEntities_.contains(sess.playerId)) {
        addPlayerEntity(sess.playerId);
    }
}

void GameInstance::addPlayerEntity(std::uint32_t playerId)
{
    static const std::array<std::uint16_t, 4> kPlayerTypes{1, 12, 13, 14};
    EntityId entity = registry_.createEntity();
    registry_.emplace<TransformComponent>(entity, TransformComponent::create(100.0F, 400.0F));
    registry_.emplace<VelocityComponent>(entity, VelocityComponent::create(0.0F, 0.0F));
    registry_.emplace<HealthComponent>(entity, HealthComponent::create(1));
    registry_.emplace<PlayerInputComponent>(entity);
    registry_.emplace<TagComponent>(entity, TagComponent::create(EntityTag::Player));
    registry_.emplace<LivesComponent>(entity, LivesComponent::create(3, 3));
    registry_.emplace<ScoreComponent>(entity, ScoreComponent::create(0));
    registry_.emplace<HitboxComponent>(entity, HitboxComponent::create(60.0F, 30.0F, 0.0F, 0.0F, true));
    registry_.emplace<BoundaryComponent>(entity, BoundaryComponent::create(0.0F, 0.0F, 1246.0F, 702.0F));
    std::size_t slot = playerEntities_.size() % kPlayerTypes.size();
    registry_.emplace<RenderTypeComponent>(entity, RenderTypeComponent::create(kPlayerTypes[slot]));
    playerEntities_[playerId] = entity;
}

void GameInstance::maybeStartGame()
{
    if (gameStarted_ || !ready())
        return;

    Logger::instance().info("[Game] All players ready, starting game");

    auto startPkt = buildGameStart(0);
    for (auto& [_, s] : sessions_) {
        sendThread_.sendTo(startPkt, s.endpoint);
        s.started = true;
    }
    auto levelPkt = buildLevelInitPacket(buildLevel());
    for (auto& [_, s] : sessions_) {
        sendThread_.sendTo(levelPkt, s.endpoint);
        s.levelSent = true;
    }
    introCinematic_.start(playerEntities_, registry_);
    gameStarted_ = true;
}

void GameInstance::startCountdown() {}

void GameInstance::updateCountdown(float) {}

std::vector<ReceivedInput> GameInstance::mapInputs(const std::vector<ReceivedInput>& inputs)
{
    std::vector<ReceivedInput> mapped;
    mapped.reserve(inputs.size());
    for (const auto& input : inputs) {
        auto sessIt = sessions_.find(endpointKey(input.from));
        if (sessIt == sessions_.end())
            continue;
        std::uint32_t playerId = sessIt->second.playerId;
        if (!playerEntities_.contains(playerId))
            continue;
        auto it = playerEntities_.find(playerId);
        if (it == playerEntities_.end())
            continue;
        ReceivedInput copy  = input;
        copy.input.playerId = it->second;
        mapped.push_back(copy);
    }
    return mapped;
}

void GameInstance::processTimeouts()
{
    ClientTimeoutEvent timeoutEvent;
    while (timeoutQueue_.tryPop(timeoutEvent)) {
        Logger::instance().warn("[Net] Client timeout: " + endpointKey(timeoutEvent.endpoint));
        onDisconnect(timeoutEvent.endpoint);
    }
}

LevelDefinition GameInstance::buildLevel() const
{
    LevelDefinition lvl{};
    lvl.levelId      = static_cast<std::uint16_t>(levelData_.levelId);
    lvl.seed         = nextSeed();
    lvl.backgroundId = levelData_.meta.backgroundId;
    lvl.musicId      = levelData_.meta.musicId;
    lvl.archetypes   = levelData_.archetypes;
    lvl.bosses.reserve(levelData_.bosses.size());
    for (const auto& [bossId, boss] : levelData_.bosses) {
        LevelBossDefinition entry{};
        entry.typeId = boss.typeId;
        entry.name   = bossId;
        entry.scaleX = boss.scale.x;
        entry.scaleY = boss.scale.y;
        lvl.bosses.push_back(std::move(entry));
    }
    return lvl;
}

bool GameInstance::ready() const
{
    if (sessions_.empty())
        return false;
    for (const auto& [_, s] : sessions_) {
        if (!s.hello || !s.join || !s.ready)
            return false;
    }
    return true;
}

std::uint32_t GameInstance::nextSeed() const
{
    std::random_device rd;
    return rd();
}

void GameInstance::onDisconnect(const IpEndpoint& endpoint)
{
    Logger::instance().info("[Net] Client disconnected: " + endpointKey(endpoint));

    auto it = sessions_.find(endpointKey(endpoint));
    if (it != sessions_.end()) {
        auto& sess = it->second;

        if (playerEntities_.contains(sess.playerId)) {
            EntityId eid = playerEntities_[sess.playerId];
            if (registry_.isAlive(eid)) {
                registry_.destroyEntity(eid);
            }
            playerEntities_.erase(sess.playerId);
        }

        sessions_.erase(it);
    }
    std::erase_if(clients_, [&](const IpEndpoint& ep) { return endpointKey(ep) == endpointKey(endpoint); });
    sendThread_.setClients(clients_);
    if (sessions_.empty()) {
        Logger::instance().info("[Game] No more clients connected, resetting game");
        resetGame();
    }
}
