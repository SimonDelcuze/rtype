#include "server/ServerRunner.hpp"

#include "Logger.hpp"
#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"
#include "server/SpawnConfig.hpp"

#include <algorithm>
#include <chrono>
#include <random>
#include <thread>

ServerApp::ServerApp(std::uint16_t port, std::atomic<bool>& runningFlag)
    : playerInputSys_(250.0F, 500.0F, 2.0F, 10), movementSys_(), monsterSpawnSys_([] {
          auto setup = buildSpawnSetupForLevel(1);
          return MonsterSpawnSystem(std::move(setup.first), std::move(setup.second));
      }()),
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
}
