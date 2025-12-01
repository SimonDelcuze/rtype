#include "Logger.hpp"
#include "Main.hpp"
#include "components/Components.hpp"
#include "ecs/Registry.hpp"
#include "events/EventBus.hpp"
#include "game/GameLoopThread.hpp"
#include "network/DeltaStatePacket.hpp"
#include "network/InputReceiveThread.hpp"
#include "network/SendThread.hpp"
#include "systems/MovementSystem.hpp"
#include "systems/PlayerInputSystem.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <functional>
#include <map>
#include <thread>

static std::atomic<bool> g_serverRunning{true};

void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        Logger::instance().info("Stop signal received...");
        g_serverRunning = false;
    }
}

static void handleNewClient(const IpEndpoint& endpoint, std::vector<IpEndpoint>& clients, SendThread& sendThread)
{
    clients.push_back(endpoint);
    sendThread.setClients(clients);
    Logger::instance().info("New client connected: " + std::to_string(endpoint.addr[0]) + "." +
                            std::to_string(endpoint.addr[1]) + "." + std::to_string(endpoint.addr[2]) + "." +
                            std::to_string(endpoint.addr[3]) + ":" + std::to_string(endpoint.port));
}

static void createPlayerEntity(std::uint32_t playerId, Registry& registry,
                               std::map<std::uint32_t, EntityId>& playerEntities)
{
    EntityId entity = registry.createEntity();
    registry.emplace<TransformComponent>(entity, TransformComponent::create(100.0F, 200.0F));
    registry.emplace<VelocityComponent>(entity, VelocityComponent::create(0.0F, 0.0F));
    registry.emplace<HealthComponent>(entity, HealthComponent::create(100));
    registry.emplace<PlayerInputComponent>(entity);
    registry.emplace<TagComponent>(entity, TagComponent::create(EntityTag::Player));
    playerEntities[playerId] = entity;
    Logger::instance().info(" Player entity created: playerId=" + std::to_string(playerId) +
                            " entityId=" + std::to_string(entity));
}

static void buildSnapshot(Registry& registry, uint32_t currentTick, DeltaStatePacket& snapshot)
{
    snapshot.header.messageType = static_cast<std::uint8_t>(MessageType::Snapshot);
    snapshot.header.sequenceId  = static_cast<std::uint16_t>(currentTick & 0xFFFF);
    snapshot.header.tickId      = currentTick;

    auto view = registry.view<TransformComponent>();
    for (EntityId id : view) {
        DeltaEntry entry;
        entry.entityId = id;

        if (registry.has<TransformComponent>(id)) {
            const auto& transform = registry.get<TransformComponent>(id);
            entry.posX            = transform.x;
            entry.posY            = transform.y;
        }
        if (registry.has<VelocityComponent>(id)) {
            const auto& velocity = registry.get<VelocityComponent>(id);
            entry.velX           = velocity.vx;
            entry.velY           = velocity.vy;
        }
        if (registry.has<HealthComponent>(id)) {
            const auto& health = registry.get<HealthComponent>(id);
            entry.hp           = health.current;
        }
        snapshot.entries.push_back(entry);
    }
}

static void processInputs(const std::vector<ReceivedInput>& inputs, Registry& registry,
                          std::map<std::uint32_t, EntityId>& playerEntities, std::vector<IpEndpoint>& clients,
                          SendThread& sendThread)
{
    for (const auto& input : inputs) {
        std::uint32_t playerId = input.input.playerId;

        bool clientFound = false;
        for (const auto& c : clients) {
            if (c.addr == input.from.addr && c.port == input.from.port) {
                clientFound = true;
                break;
            }
        }
        if (!clientFound) {
            handleNewClient(input.from, clients, sendThread);
        }
        if (!playerEntities.contains(playerId)) {
            createPlayerEntity(playerId, registry, playerEntities);
        }
    }
}

static void runMainLoop(ThreadSafeQueue<ClientTimeoutEvent>& timeoutQueue)
{
    Logger::instance().info("");
    Logger::instance().info("  SERVER RUNNING");
    Logger::instance().info("  Port: 12345");
    Logger::instance().info("  Press Ctrl+C to stop");
    Logger::instance().info("");

    while (g_serverRunning) {
        ClientTimeoutEvent timeoutEvent;
        while (timeoutQueue.tryPop(timeoutEvent)) {
            std::array<char, 32> addrBuf{};
            snprintf(addrBuf.data(), addrBuf.size(), "%d.%d.%d.%d:%d", timeoutEvent.endpoint.addr[0],
                     timeoutEvent.endpoint.addr[1], timeoutEvent.endpoint.addr[2], timeoutEvent.endpoint.addr[3],
                     timeoutEvent.endpoint.port);
            Logger::instance().warn("Client timeout: " + std::string(addrBuf.data()));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

static void shutdownServer(GameLoopThread& gameLoop, SendThread& sendThread, InputReceiveThread& receiveThread)
{
    Logger::instance().info("Stopping server...");
    gameLoop.stop();
    Logger::instance().info(" Game Loop stopped");
    sendThread.stop();
    Logger::instance().info(" Send Thread stopped");
    receiveThread.stop();
    Logger::instance().info(" Receive Thread stopped");
    Logger::instance().info("");
    Logger::instance().info("  Server stopped cleanly");
    Logger::instance().info("");
}

void run_server(void)
{
    Logger::instance().info("");
    Logger::instance().info("         R-Type Server v1.0             ");
    Logger::instance().info("");
    std::signal(SIGINT, signalHandler);

    Registry registry;
    EventBus eventBus;
    std::map<std::uint32_t, EntityId> playerEntities;
    std::vector<IpEndpoint> clients;

    PlayerInputSystem playerInputSys(250.0F, 500.0F, 2.0F, 10);
    MovementSystem movementSys;
    Logger::instance().info(" Registry and ECS Systems created");

    ThreadSafeQueue<ReceivedInput> inputQueue;
    ThreadSafeQueue<ClientTimeoutEvent> timeoutQueue;

    InputReceiveThread receiveThread(IpEndpoint{.addr = {0, 0, 0, 0}, .port = 12345}, inputQueue, &timeoutQueue,
                                     std::chrono::seconds(30));
    if (!receiveThread.start()) {
        Logger::instance().error(" Failed to start Receive Thread");
        return;
    }
    Logger::instance().info(" Receive Thread started on port 12345");

    SendThread sendThread(IpEndpoint{.addr = {0, 0, 0, 0}, .port = 0}, clients, 60.0);
    if (!sendThread.start()) {
        Logger::instance().error(" Failed to start Send Thread");
        receiveThread.stop();
        return;
    }
    Logger::instance().info(" Send Thread started");

    uint32_t currentTick = 0;
    auto tickCallback    = [&](const std::vector<ReceivedInput>& inputs) {
        processInputs(inputs, registry, playerEntities, clients, sendThread);
        playerInputSys.update(registry, inputs);
        movementSys.update(registry, 1.0F / 60.0F);

        DeltaStatePacket snapshot;
        buildSnapshot(registry, currentTick, snapshot);
        if (!snapshot.entries.empty()) {
            sendThread.publish(snapshot);
        }
        currentTick++;
    };

    GameLoopThread gameLoop(inputQueue, tickCallback, 60.0);
    if (!gameLoop.start()) {
        Logger::instance().error(" Failed to start Game Loop");
        sendThread.stop();
        receiveThread.stop();
        return;
    }
    Logger::instance().info(" Game Loop started (60 FPS)");

    runMainLoop(timeoutQueue);
    shutdownServer(gameLoop, sendThread, receiveThread);
}
