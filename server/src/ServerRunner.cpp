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
    auto mapped = mapInputs(inputs);
    playerInputSys_.update(registry_, mapped);
    movementSys_.update(registry_, 1.0F / kTickRate);
    auto snapshot = buildSnapshotPacket(registry_, currentTick_);
    if (!snapshot.empty())
        sendThread_.publish(snapshot);
    currentTick_++;
}
