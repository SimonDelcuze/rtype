#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"
#include "game/GameLoopThread.hpp"
#include "network/InputReceiveThread.hpp"
#include "network/SendThread.hpp"
#include "server/LevelScript.hpp"
#include "server/Packets.hpp"
#include "server/Session.hpp"
#include "systems/BoundarySystem.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/DestructionSystem.hpp"
#include "systems/EnemyShootingSystem.hpp"
#include "systems/MonsterMovementSystem.hpp"
#include "systems/MonsterSpawnSystem.hpp"
#include "systems/MovementSystem.hpp"
#include "systems/ObstacleSpawnSystem.hpp"
#include "systems/PlayerInputSystem.hpp"

#include <atomic>
#include <cstdint>
#include <events/EventBus.hpp>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class ServerApp
{
  public:
    ServerApp(std::uint16_t port, std::atomic<bool>& runningFlag);
    bool start();
    void run();
    void stop();

  private:
    static constexpr double kTickRate = 60.0;

    void handleControl();
    void handleControlMessage(const ControlEvent& ctrl);
    void onJoin(ClientSession& sess, const ControlEvent& ctrl);
    void addPlayerEntity(std::uint32_t playerId);
    void maybeStartGame();
    void tick(const std::vector<ReceivedInput>& inputs);
    void updateSystems(float deltaTime, const std::vector<ReceivedInput>& inputs);
    std::vector<EntityId> collectDeadEntities();
    void broadcastDestructions(const std::vector<EntityId>& toDestroy);
    std::unordered_set<EntityId> collectCurrentEntities();
    void syncEntityLifecycle(const std::unordered_set<EntityId>& current);
    void sendSnapshots();
    std::vector<ReceivedInput> mapInputs(const std::vector<ReceivedInput>& inputs);
    void processTimeouts();
    LevelDefinition buildLevel() const;
    bool ready() const;
    void startCountdown();
    void updateCountdown(float dt);

    void cleanupOffscreenEntities();
    void cleanupExpiredMissiles(float deltaTime);
    void logCollisions(const std::vector<Collision>& collisions);
    std::string getEntityTagName(EntityId id) const;
    std::uint32_t nextSeed() const;
    void resetGame();
    void onDisconnect(const IpEndpoint& endpoint);

    Registry registry_;
    std::map<std::uint32_t, EntityId> playerEntities_;
    std::vector<IpEndpoint> clients_;
    std::unordered_map<std::string, ClientSession> sessions_;
    EventBus eventBus_;
    LevelScript levelScript_;
    PlayerInputSystem playerInputSys_;
    MovementSystem movementSys_;
    MonsterSpawnSystem monsterSpawnSys_;
    ObstacleSpawnSystem obstacleSpawnSys_;
    MonsterMovementSystem monsterMovementSys_;
    EnemyShootingSystem enemyShootingSys_;
    CollisionSystem collisionSys_;
    DamageSystem damageSys_;
    DestructionSystem destructionSys_;
    BoundarySystem boundarySys_;
    ThreadSafeQueue<ReceivedInput> inputQueue_;
    ThreadSafeQueue<ControlEvent> controlQueue_;
    ThreadSafeQueue<ClientTimeoutEvent> timeoutQueue_;
    InputReceiveThread receiveThread_;
    SendThread sendThread_;
    GameLoopThread gameLoop_;
    std::uint32_t currentTick_{0};
    bool gameStarted_{false};
    bool countdownActive_{false};
    float countdownTimer_{3.0F};
    int lastCountdownValue_{4};
    std::uint32_t nextPlayerId_{1};
    std::atomic<bool>* running_{nullptr};
    std::unordered_set<EntityId> knownEntities_;
};
