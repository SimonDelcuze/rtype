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
      monsterSpawnSys_(MonsterSpawnConfig{.spawnInterval = 2.0F, .spawnX = 1300.0F, .yMin = 50.0F, .yMax = 550.0F},
                       {MovementComponent::linear(100.0F), MovementComponent::zigzag(100.0F, 50.0F, 2.0F),
                        MovementComponent::sine(100.0F, 30.0F, 1.5F, 0.0F)}),
      receiveThread_(IpEndpoint{.addr = {0, 0, 0, 0}, .port = port}, inputQueue_, controlQueue_, &timeoutQueue_,
                     std::chrono::seconds(30)),
      sendThread_(IpEndpoint{.addr = {0, 0, 0, 0}, .port = 0}, clients_, kTickRate),
      gameLoop_(
          inputQueue_, [this](const std::vector<ReceivedInput>& inputs) { tick(inputs); }, kTickRate),
      running_(&runningFlag)
{}

bool ServerApp::start()
{
    if (!receiveThread_.start())
        return false;
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
    if (!gameStarted_)
        return;
    float deltaTime = 1.0F / kTickRate;
    monsterSpawnSys_.update(registry_, deltaTime);
    auto mapped = mapInputs(inputs);
    playerInputSys_.update(registry_, mapped);
    monsterMovementSys_.update(registry_, deltaTime);
    movementSys_.update(registry_, deltaTime);
    auto snapshot = buildSnapshotPacket(registry_, currentTick_);
    if (!snapshot.empty())
        sendThread_.publish(snapshot);
    currentTick_++;
}
