#include "Logger.hpp"
#include "server/ServerRunner.hpp"

#include <random>

void ServerApp::handleControl()
{
    ControlEvent ctrl{};
    while (controlQueue_.tryPop(ctrl)) {
        handleControlMessage(ctrl);
    }
}

void ServerApp::handleControlMessage(const ControlEvent& ctrl)
{
    auto key  = endpointKey(ctrl.from);
    auto type = ctrl.header.messageType;

    if (gameStarted_) {
        if (type == static_cast<std::uint8_t>(MessageType::ClientHello) ||
            type == static_cast<std::uint8_t>(MessageType::ClientJoinRequest)) {
            Logger::instance().warn("Rejecting connection - game already started");
            sendThread_.sendTo(buildJoinDeny(ctrl.header.sequenceId), ctrl.from);
            return;
        }
    }

    auto& sess    = sessions_[key];
    sess.endpoint = ctrl.from;
    if (sess.playerId == 0)
        sess.playerId = static_cast<std::uint32_t>(sessions_.size());

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

void ServerApp::onJoin(ClientSession& sess, const ControlEvent& ctrl)
{
    if (gameStarted_) {
        Logger::instance().warn("Rejecting join request - game already started");
        sendThread_.sendTo(buildJoinDeny(ctrl.header.sequenceId), ctrl.from);
        return;
    }

    sess.join = true;
    sendThread_.sendTo(buildJoinAccept(ctrl.header.sequenceId), ctrl.from);
    clients_.push_back(ctrl.from);
    sendThread_.setClients(clients_);
    addPlayerEntity(sess.playerId);
}

void ServerApp::addPlayerEntity(std::uint32_t playerId)
{
    EntityId entity = registry_.createEntity();
    registry_.emplace<TransformComponent>(entity, TransformComponent::create(100.0F, 400.0F));
    registry_.emplace<VelocityComponent>(entity, VelocityComponent::create(0.0F, 0.0F));
    registry_.emplace<HealthComponent>(entity, HealthComponent::create(100));
    registry_.emplace<PlayerInputComponent>(entity);
    registry_.emplace<TagComponent>(entity, TagComponent::create(EntityTag::Player));
    registry_.emplace<HitboxComponent>(entity, HitboxComponent::create(60.0F, 30.0F, 0.0F, 0.0F, true));
    playerEntities_[playerId] = entity;
}

void ServerApp::maybeStartGame()
{
    if (gameStarted_ || !ready())
        return;

    Logger::instance().info("All players ready, starting game");

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
    gameStarted_ = true;
}

void ServerApp::startCountdown() {}

void ServerApp::updateCountdown(float) {}

std::vector<ReceivedInput> ServerApp::mapInputs(const std::vector<ReceivedInput>& inputs)
{
    std::vector<ReceivedInput> mapped;
    mapped.reserve(inputs.size());
    for (const auto& input : inputs) {
        std::uint32_t playerId = input.input.playerId;
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

void ServerApp::processTimeouts()
{
    ClientTimeoutEvent timeoutEvent;
    while (timeoutQueue_.tryPop(timeoutEvent)) {
        Logger::instance().warn("Client timeout");
    }
}

LevelDefinition ServerApp::buildLevel() const
{
    LevelDefinition lvl{};
    lvl.levelId      = 1;
    lvl.seed         = nextSeed();
    lvl.backgroundId = "space_background";
    lvl.musicId      = "theme_music";
    lvl.archetypes.push_back(LevelArchetype{1, "player_ship", "player1", 0});
    lvl.archetypes.push_back(LevelArchetype{2, "mob1", "left", 0});
    lvl.archetypes.push_back(LevelArchetype{3, "bullet", "bullet_basic", 0});
    lvl.archetypes.push_back(LevelArchetype{4, "bullet", "bullet_charge_lvl1", 0});
    lvl.archetypes.push_back(LevelArchetype{5, "bullet", "bullet_charge_lvl2", 0});
    lvl.archetypes.push_back(LevelArchetype{6, "bullet", "bullet_charge_lvl3", 0});
    lvl.archetypes.push_back(LevelArchetype{7, "bullet", "bullet_charge_lvl4", 0});
    lvl.archetypes.push_back(LevelArchetype{8, "bullet", "bullet_charge_lvl5", 0});
    return lvl;
}

bool ServerApp::ready() const
{
    if (sessions_.empty())
        return false;
    for (const auto& [_, s] : sessions_) {
        if (!s.hello || !s.join || !s.ready)
            return false;
    }
    return true;
}

std::uint32_t ServerApp::nextSeed() const
{
    std::random_device rd;
    return rd();
}

void ServerApp::onDisconnect(const IpEndpoint& endpoint)
{
    Logger::instance().info("Client disconnected: " + endpointKey(endpoint));

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
        Logger::instance().info("No more clients connected, resetting game");
        resetGame();
    }
}
