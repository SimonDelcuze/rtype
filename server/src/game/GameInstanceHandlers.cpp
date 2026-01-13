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

    bool isAuthoritative = (ctrl.from.addr[0] == 0 && ctrl.from.addr[1] == 0 && ctrl.from.addr[2] == 0 &&
                            ctrl.from.addr[3] == 0 && ctrl.from.port == 0);

    if (isAuthoritative) {
        if (type == static_cast<std::uint8_t>(MessageType::RoomForceStart)) {
            if (ctrl.data.size() >= 5) {
                onSetPlayerCount(ctrl.data[4]);
            }
            onForceStart(0, true);
        }
        return;
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
    } else if (type == static_cast<std::uint8_t>(MessageType::RoomForceStart)) {
        onForceStart(sess.playerId, false);
    } else if (type == static_cast<std::uint8_t>(MessageType::RoomSetPlayerCount)) {
        if (ctrl.data.size() >= PacketHeader::kSize + 1) {
            std::uint8_t playerCount = ctrl.data[PacketHeader::kSize];
            onSetPlayerCount(playerCount);
        }
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

    if (playerEntities_.empty()) {
        sess.role = PlayerRole::Owner;
        Logger::instance().info("[Room] Player " + std::to_string(sess.playerId) + " is the room owner");
    }

    sess.join = true;
    if (forceStarted_) {
        sess.hello = true;
        sess.ready = true;
        Logger::instance().info("[Room] Player " + std::to_string(sess.playerId) + " auto-ready (force started)");
    }

    sendThread_.sendTo(buildJoinAccept(ctrl.header.sequenceId, sess.playerId), ctrl.from);

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

    if (forceStarted_) {
        maybeStartGame();
    }
}

void GameInstance::addPlayerEntity(std::uint32_t playerId)
{
    static const std::array<std::uint16_t, 4> kPlayerTypes{1, 12, 13, 14};
    EntityId entity = registry_.createEntity();
    registry_.emplace<BoundaryComponent>(entity, BoundaryComponent::create(0.0F, 0.0F, 1246.0F, 702.0F));
    auto spawn = respawnPosition(entity);
    registry_.emplace<TransformComponent>(entity, TransformComponent::create(spawn.x, spawn.y));
    registry_.emplace<VelocityComponent>(entity, VelocityComponent::create(0.0F, 0.0F));
    registry_.emplace<HealthComponent>(entity, HealthComponent::create(1));
    registry_.emplace<PlayerInputComponent>(entity);
    registry_.emplace<TagComponent>(entity, TagComponent::create(EntityTag::Player));
    std::uint8_t lives = computePlayerLives();
    registry_.emplace<LivesComponent>(entity, LivesComponent::create(lives, lives));
    registry_.emplace<ScoreComponent>(entity, ScoreComponent::create(0));
    registry_.emplace<HitboxComponent>(entity, HitboxComponent::create(60.0F, 30.0F, 0.0F, 0.0F, true));
    registry_.emplace<OwnershipComponent>(entity, OwnershipComponent::create(playerId));
    std::size_t slot = playerEntities_.size() % kPlayerTypes.size();
    registry_.emplace<RenderTypeComponent>(entity, RenderTypeComponent::create(kPlayerTypes[slot]));
    playerEntities_[playerId] = entity;
}

void GameInstance::onForceStart(std::uint32_t playerId, bool authoritative)
{
    if (gameStarted_) {
        Logger::instance().warn("[Game] Cannot force start - game already started");
        return;
    }

    if (!authoritative && !isOwner(playerId)) {
        Logger::instance().warn("[Game] Player " + std::to_string(playerId) + " is not owner, cannot force start");
        return;
    }

    if (authoritative) {
        Logger::instance().info("[Game] Authoritative force start command received (from Lobby)");
        forceStarted_ = true;
    } else {
        Logger::instance().info("[Game] Force start command from Player " + std::to_string(playerId) +
                                " (isOwner=" + std::string(isOwner(playerId) ? "TRUE" : "FALSE") + ")");
    }

    for (auto& [key, s] : sessions_) {
        Logger::instance().info("[Game] Marking session " + key + " (PlayerId=" + std::to_string(s.playerId) +
                                ") as auto-ready");
        s.ready = true;
    }

    maybeStartGame();
}

void GameInstance::onSetPlayerCount(std::uint8_t count)
{
    expectedPlayerCount_ = count;
    Logger::instance().info("[Game] Expected player count set to " + std::to_string(count));
}

void GameInstance::maybeStartGame()
{
    if (gameStarted_) {
        return;
    }

    if (!ready()) {
        return;
    }

    Logger::instance().info("[Game] All players ready, starting simulation for Room " + std::to_string(roomId_));

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
    if (sessions_.empty()) {
        return false;
    }

    if (expectedPlayerCount_ > 0 && sessions_.size() < expectedPlayerCount_) {
        Logger::instance().info("[Game] ready() = FALSE: sessions.size(" + std::to_string(sessions_.size()) +
                                ") < expected(" + std::to_string(expectedPlayerCount_) + ")");
        return false;
    }

    for (const auto& [key, s] : sessions_) {
        if (!s.hello || !s.join || !s.ready) {
            Logger::instance().info("[Game] ready() = FALSE: player " + std::to_string(s.playerId) + " (" + key +
                                    ") not fully ready: hello=" + std::string(s.hello ? "Y" : "N") + " join=" +
                                    std::string(s.join ? "Y" : "N") + " ready=" + std::string(s.ready ? "Y" : "N"));
            return false;
        }
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

    if (gameEnded_) {
        Logger::instance().info("[Game] Game ended and player disconnected, resetting for retry");
        resetGame();
    } else if (sessions_.empty()) {
        Logger::instance().info("[Game] No more clients connected, resetting game");
        resetGame();
    }
}
