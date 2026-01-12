#pragma once

#include "components/Components.hpp"
#include "core/Session.hpp"
#include "ecs/Registry.hpp"
#include "game/GameLoopThread.hpp"
#include "levels/IntroCinematic.hpp"
#include "levels/LevelData.hpp"
#include "levels/LevelDirector.hpp"
#include "levels/LevelLoader.hpp"
#include "levels/LevelSpawnSystem.hpp"
#include "lobby/RoomConfig.hpp"
#include "network/InputReceiveThread.hpp"
#include "network/NetworkBridge.hpp"
#include "network/Packets.hpp"
#include "network/SendThread.hpp"
#include "replication/ReplicationManager.hpp"
#include "rollback/DesyncDetector.hpp"
#include "rollback/RollbackManager.hpp"
#include "simulation/GameWorld.hpp"
#include "simulation/PlayerCommand.hpp"
#include "systems/BoundarySystem.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/DestructionSystem.hpp"
#include "systems/EnemyShootingSystem.hpp"
#include "systems/MonsterMovementSystem.hpp"
#include "systems/MovementSystem.hpp"
#include "systems/PlayerBoundsSystem.hpp"
#include "systems/PlayerInputSystem.hpp"
#include "systems/ScoreSystem.hpp"
#include "systems/WalkerShotSystem.hpp"

#include <atomic>
#include <cstdint>
#include <events/EventBus.hpp>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class GameInstance
{
  public:
    GameInstance(std::uint32_t roomId, std::uint16_t port, std::atomic<bool>& runningFlag);
    void setRoomConfig(const RoomConfig& config);
    bool start();
    void run();
    void stop(const std::string& reason = "Room closed");
    void notifyDisconnection(const std::string& reason);
    void broadcast(const std::string& message);

    std::uint32_t getRoomId() const
    {
        return roomId_;
    }
    std::uint16_t getPort() const
    {
        return port_;
    }
    std::size_t getPlayerCount() const;
    std::vector<ClientSession> getSessions() const;
    void kickPlayer(std::uint32_t playerId);
    void kickPlayer(std::uint32_t playerId, const std::string& reason);
    void banPlayer(std::uint32_t playerId, const std::string& reason);
    void promoteToAdmin(std::uint32_t playerId);
    void demoteFromAdmin(std::uint32_t playerId);
    bool isOwner(std::uint32_t playerId) const;
    bool isAdmin(std::uint32_t playerId) const;
    bool canKick(std::uint32_t kickerId, std::uint32_t targetId) const;
    bool canPromoteAdmin(std::uint32_t promoterId) const;
    bool isGameStarted() const
    {
        return gameStarted_;
    }
    bool isEmpty() const;

    void handleInput(const ReceivedInput& input);
    void handleControlEvent(const ControlEvent& ctrl);
    void handleTimeout(const ClientTimeoutEvent& timeout);

  private:
    static constexpr double kTickRate                 = 60.0;
    static constexpr std::uint32_t kFullStateInterval = 60;

    void handleControl();
    void handleControlMessage(const ControlEvent& ctrl);
    void onJoin(ClientSession& sess, const ControlEvent& ctrl);
    void onForceStart(std::uint32_t playerId, bool authoritative = false);
    void onSetPlayerCount(std::uint8_t count);
    void addPlayerEntity(std::uint32_t playerId);
    void maybeStartGame();
    void tick(const std::vector<ReceivedInput>& inputs);
    void updateNetworkStats(float dt);
    void updateGameplay(float dt, const std::vector<ReceivedInput>& inputs);
    void updateSystems(float deltaTime, const std::vector<ReceivedInput>& inputs);
    std::vector<EntityId> collectDeadEntities();
    void broadcastDestructions(const std::vector<EntityId>& toDestroy);
    std::vector<PlayerCommand> convertInputsToCommands(const std::vector<ReceivedInput>& inputs) const;
    std::unordered_set<EntityId> collectCurrentEntities();
    void sendSnapshots();
    void logSnapshotSummary(std::size_t totalBytes, std::size_t payloadSize, bool forceFull);
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
    void applyConfig();
    std::uint8_t computePlayerLives() const;

    void updateRespawnTimers(float deltaTime);
    void updateInvincibilityTimers(float deltaTime);
    void handleDeathAndRespawn();
    void spawnPlayerDeathFx(float x, float y);
    void sendLevelEvents(const std::vector<DispatchedEvent>& events);
    void sendSegmentState();
    Vec2f respawnPosition(EntityId entityId) const;
    void respawnPlayer(EntityId entityId);

    void logInfo(const std::string& msg) const;
    void logWarn(const std::string& msg) const;
    void logError(const std::string& msg) const;

    std::uint32_t roomId_;
    std::uint16_t port_;
    GameWorld world_;
    Registry& registry_;
    RoomConfig roomConfig_{RoomConfig::preset(RoomDifficulty::Hell)};
    std::map<std::uint32_t, EntityId> playerEntities_;
    std::vector<IpEndpoint> clients_;
    std::unordered_map<std::string, ClientSession> sessions_;
    EventBus eventBus_;
    LevelData levelData_;
    std::unique_ptr<LevelDirector> levelDirector_;
    std::unique_ptr<LevelSpawnSystem> levelSpawnSys_;
    bool levelLoaded_{false};
    LevelSpawnSystem::SpawnScaling spawnScaling_{};
    PlayerInputSystem playerInputSys_;
    MovementSystem movementSys_;
    MonsterMovementSystem monsterMovementSys_;
    EnemyShootingSystem enemyShootingSys_;
    WalkerShotSystem walkerShotSys_;
    CollisionSystem collisionSys_;
    DamageSystem damageSys_;
    ScoreSystem scoreSys_;
    DestructionSystem destructionSys_;
    BoundarySystem boundarySys_;
    PlayerBoundsSystem playerBoundsSys_;
    IntroCinematic introCinematic_;
    ThreadSafeQueue<ReceivedInput> inputQueue_;
    ThreadSafeQueue<ControlEvent> controlQueue_;
    ThreadSafeQueue<ClientTimeoutEvent> timeoutQueue_;
    InputReceiveThread receiveThread_;
    SendThread sendThread_;
    GameLoopThread gameLoop_;
    std::uint32_t currentTick_{0};
    bool gameStarted_{false};
    bool forceStarted_{false};
    bool countdownActive_{false};
    float countdownTimer_{3.0F};
    int lastCountdownValue_{4};
    std::int32_t lastSegmentIndex_{-1};
    std::uint32_t nextPlayerId_{1};
    std::uint8_t expectedPlayerCount_{0};
    std::atomic<bool>* running_{nullptr};
    NetworkBridge networkBridge_;
    ReplicationManager replicationManager_;
    RollbackManager rollbackManager_;
    DesyncDetector desyncDetector_;

    void captureStateSnapshot();
    void handleDesync(const DesyncInfo& desyncInfo);
};
