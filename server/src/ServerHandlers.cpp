#include "server/ServerRunner.hpp"

#include "Logger.hpp"

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
    auto key   = endpointKey(ctrl.from);
    auto& sess = sessions_[key];
    sess.endpoint = ctrl.from;
    if (sess.playerId == 0)
        sess.playerId = static_cast<std::uint32_t>(sessions_.size());

    auto type = ctrl.header.messageType;
    if (type == static_cast<std::uint8_t>(MessageType::ClientHello)) {
        sess.hello = true;
        sendThread_.sendTo(buildServerHello(ctrl.header.sequenceId), ctrl.from);
    } else if (type == static_cast<std::uint8_t>(MessageType::ClientJoinRequest)) {
        onJoin(sess, ctrl);
    } else if (type == static_cast<std::uint8_t>(MessageType::ClientReady)) {
        sess.ready = true;
    } else if (type == static_cast<std::uint8_t>(MessageType::ClientPing)) {
        sendThread_.sendTo(buildPong(ctrl.header), ctrl.from);
    }
}

void ServerApp::onJoin(ClientSession& sess, const ControlEvent& ctrl)
{
    sess.join = true;
    sendThread_.sendTo(buildJoinAccept(ctrl.header.sequenceId), ctrl.from);
    clients_.push_back(ctrl.from);
    sendThread_.setClients(clients_);
    addPlayerEntity(sess.playerId);
}

void ServerApp::addPlayerEntity(std::uint32_t playerId)
{
    EntityId entity = registry_.createEntity();
    registry_.emplace<TransformComponent>(entity, TransformComponent::create(100.0F, 200.0F));
    registry_.emplace<VelocityComponent>(entity, VelocityComponent::create(0.0F, 0.0F));
    registry_.emplace<HealthComponent>(entity, HealthComponent::create(100));
    registry_.emplace<PlayerInputComponent>(entity);
    registry_.emplace<TagComponent>(entity, TagComponent::create(EntityTag::Player));
    playerEntities_[playerId] = entity;
}

void ServerApp::maybeStartGame()
{
    if (gameStarted_ || !ready())
        return;
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
        ReceivedInput copy = input;
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
    lvl.archetypes.push_back(LevelArchetype{2, "enemy_ship", "enemy1", 0});
    lvl.archetypes.push_back(LevelArchetype{3, "bullet", "bullet", 0});
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
