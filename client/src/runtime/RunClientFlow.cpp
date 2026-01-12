#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "animation/AnimationManifest.hpp"
#include "audio/SoundManager.hpp"
#include "auth/AuthResult.hpp"
#include "ecs/Registry.hpp"
#include "events/EventBus.hpp"
#include "events/GameEvents.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/abstraction/Event.hpp"
#include "input/InputMapper.hpp"
#include "level/EntityTypeSetup.hpp"
#include "level/LevelState.hpp"
#include "network/GameEndPacket.hpp"
#include "network/LobbyConnection.hpp"
#include "network/PacketHeader.hpp"
#include "scheduler/ClientScheduler.hpp"
#include "scheduler/GameLoop.hpp"
#include "systems/ButtonSystem.hpp"
#include "systems/NotificationSystem.hpp"
#include "systems/RenderSystem.hpp"
#include "ui/ConnectionMenu.hpp"
#include "ui/GameOverMenu.hpp"
#include "ui/MenuRunner.hpp"
#include "ui/ModeSelectMenu.hpp"

#include <chrono>
#include <iostream>

std::optional<AuthResult> showAuthenticationMenu(Window& window, FontManager& fontManager,
                                                 TextureManager& textureManager, LobbyConnection& lobbyConn,
                                                 ThreadSafeQueue<NotificationData>& broadcastQueue);

std::optional<std::pair<IpEndpoint, RoomType>>
resolveServerEndpoint(const ClientOptions& options, Window& window, FontManager& fontManager,
                      TextureManager& textureManager, std::string& errorMessage,
                      ThreadSafeQueue<NotificationData>& broadcastQueue, std::optional<IpEndpoint>& lastLobbyEndpoint,
                      std::uint32_t& outUserId)
{
    while (window.isOpen() && g_running && !g_forceExit.load()) {
        if (!options.useDefault && !lastLobbyEndpoint.has_value()) {
            Logger::instance().info("[Nav] Showing server selection menu");
            auto ep = showConnectionMenu(window, fontManager, textureManager, errorMessage, broadcastQueue);
            if (!ep.has_value()) {
                return std::nullopt;
            }
            lastLobbyEndpoint = ep;
        } else if (options.useDefault && !lastLobbyEndpoint.has_value()) {
            Logger::instance().info("Using default lobby: 127.0.0.1:50010");
            lastLobbyEndpoint = IpEndpoint::v4(127, 0, 0, 1, 50010);
        }

        IpEndpoint lobbyEp = *lastLobbyEndpoint;
        Logger::instance().info("[Nav] Using lobby endpoint: port " + std::to_string(lobbyEp.port));
        bool backToServerSelect = false;

        while (window.isOpen() && !backToServerSelect && g_running && !g_forceExit.load()) {
            Logger::instance().info("[Auth] Starting authentication flow");
            LobbyConnection conn(lobbyEp, g_running);
            if (!conn.connect() || !conn.ping()) {
                Logger::instance().warn("[Nav] Failed to reach lobby server at port " + std::to_string(lobbyEp.port));
                backToServerSelect = true;
                lastLobbyEndpoint.reset();
                errorMessage = "Could not reach server";
                if (options.useDefault) {
                    return std::nullopt;
                }
                continue;
            }

            auto authResult = showAuthenticationMenu(window, fontManager, textureManager, conn, broadcastQueue);

            if (!authResult.has_value()) {
                Logger::instance().info("[Nav] Authentication cancelled/failed, returning to server selection");
                backToServerSelect = true;
                lastLobbyEndpoint.reset();
                if (options.useDefault) {
                    return std::nullopt;
                }
                continue;
            }

            Logger::instance().info("[Auth] User authenticated: " + authResult->username);
            outUserId = authResult->userId;

            bool stayingInLobbyFlow = true;
            while (stayingInLobbyFlow && window.isOpen() && g_running && !g_forceExit.load()) {
                Logger::instance().info("[Nav] Showing mode selection");
                MenuRunner modeRunner(window, fontManager, textureManager, g_running, broadcastQueue);
                auto modeRes = modeRunner.runAndGetResult<ModeSelectMenu>();
                if (modeRes.backRequested) {
                    Logger::instance().info("[Nav] Mode selection cancelled (Back), returning to login");
                    stayingInLobbyFlow = false;
                    continue;
                }

                if (modeRes.exitRequested) {
                    Logger::instance().info("[Nav] Mode selection exit requested");
                    backToServerSelect = true;
                    lastLobbyEndpoint.reset();
                    stayingInLobbyFlow = false;
                    continue;
                }
                RoomType targetRoomType = modeRes.selected;

                Logger::instance().info("[Nav] Showing lobby menu");
                bool serverLost = false;
                auto gameEp     = showLobbyMenuAndGetGameEndpoint(window, lobbyEp, targetRoomType, fontManager,
                                                                  textureManager, broadcastQueue, &conn, serverLost);

                if (gameEp.has_value()) {
                    return std::make_pair(*gameEp, targetRoomType);
                }

                if (serverLost) {
                    Logger::instance().warn("[Nav] Server connection lost in lobby");
                    backToServerSelect = true;
                    lastLobbyEndpoint.reset();
                    errorMessage       = "Connection lost to server";
                    stayingInLobbyFlow = false;
                    continue;
                }

                Logger::instance().info("[Nav] Back from lobby, returning to mode selection");
            }
        }
    }

    return std::nullopt;
}

std::optional<int> handleJoinFailure(JoinResult joinResult, Window& window, const ClientOptions& options,
                                     NetPipelines& net, std::thread& welcomeThread, std::atomic<bool>& handshakeDone,
                                     std::string& errorMessage, ThreadSafeQueue<NotificationData>& broadcastQueue)
{
    (void) errorMessage;
    if (joinResult == JoinResult::Denied) {
        Logger::instance().error("Connection rejected - game already in progress!");
        stopNetwork(net, welcomeThread, handshakeDone);
        broadcastQueue.push(NotificationData{"Connection rejected - game in progress!", 5.0F});
        net.joinDenied.store(false);
        net.joinAccepted.store(false);
        if (options.useDefault) {
            showErrorMessage(window, "Connection rejected - game in progress!");
            return 1;
        }
        return std::nullopt;
    }

    if (joinResult == JoinResult::Timeout) {
        Logger::instance().error("Server did not respond - connection timeout");
        stopNetwork(net, welcomeThread, handshakeDone);
        broadcastQueue.push(NotificationData{"Server did not respond - timeout", 5.0F});
        if (options.useDefault) {
            showErrorMessage(window, "Server did not respond - timeout");
            return 1;
        }
        return std::nullopt;
    }

    return std::nullopt;
}

namespace
{
    struct GameState
    {
        bool gameOverTriggered = false;
        int finalScore         = 0;
        bool victory           = false;
    };

    void sendDisconnectPacket(const IpEndpoint& serverEndpoint, NetPipelines& net)
    {
        if (g_forceExit.load()) {
            return;
        }
        if (net.socket && g_running == false) {
            PacketHeader header{};
            header.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
            header.messageType = static_cast<std::uint8_t>(MessageType::ClientDisconnect);
            header.sequenceId  = 0;
            header.payloadSize = 0;
            auto packet        = header.encode();
            net.socket->sendTo(packet.data(), packet.size(), serverEndpoint);
        }
    }

    void runMainGameLoop(Window& window, GameLoop& gameLoop, Registry& registry, EventBus& eventBus,
                         InputMapper& mapper, ButtonSystem& buttonSystem, NetPipelines& net, std::string& errorMessage,
                         bool& disconnected, bool& serverLost, ThreadSafeQueue<NotificationData>& broadcastQueue,
                         const IpEndpoint& serverEndpoint)
    {
        auto onEvent = [&](const Event& event) {
            mapper.handleEvent(event);
            buttonSystem.handleEvent(registry, event);
        };
        (void) errorMessage;
        (void) broadcastQueue;

        bool sessionRunning = true;
        auto lastTime       = std::chrono::steady_clock::now();
        while (window.isOpen() && g_running && sessionRunning) {
            window.pollEvents([&](const Event& event) {
                onEvent(event);
                if (event.type == EventType::Closed) {
                    g_running = false;
                    window.close();
                }
            });

            if (net.handler) {
                net.handler->poll();
                if (net.handler->getLastPacketAge() > 5.0F) {
                    Logger::instance().warn("[Net] Server timeout detected (5s)");
                    disconnected   = true;
                    sessionRunning = false;
                    net.disconnectEvents.push("Server timeout");
                }

                GameEndPacket gameEndPkt;
                while (net.handler->getGameEndQueue().tryPop(gameEndPkt)) {
                    Logger::instance().info("[RunClientFlow] Popped GameEndPacket from queue. Emitting event.");
                    eventBus.emit(GameOverEvent{gameEndPkt.victory, gameEndPkt.finalScore, 1});
                }
            }

            std::string disconnectMsg;
            if (net.disconnectEvents.tryPop(disconnectMsg)) {
                Logger::instance().warn("[Net] Disconnected from server: " + disconnectMsg);
                disconnected   = true;
                sessionRunning = false;

                if (disconnectMsg == "Server disconnected" || disconnectMsg == "Server timeout") {
                    serverLost = true;
                }

                Logger::instance().info("[Redirection] Session termination triggered. Reason: " + disconnectMsg);
            }

            auto currentTime                     = std::chrono::steady_clock::now();
            std::chrono::duration<float> elapsed = currentTime - lastTime;
            lastTime                             = currentTime;
            const float deltaTime                = std::min(elapsed.count(), 0.1F);

            static float pingTimer = 0.0F;
            pingTimer += deltaTime;
            if (pingTimer >= 2.0F) {
                pingTimer = 0.0F;
                PacketHeader header{};
                header.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
                header.messageType = static_cast<std::uint8_t>(MessageType::ClientPing);
                auto packet        = header.encode();
                if (net.socket) {
                    Logger::instance().info("[Heartbeat] Sending ping to game server...");
                    net.socket->sendTo(packet.data(), packet.size(), serverEndpoint);
                }
            }

            window.setColorFilter(g_colorFilterMode);

            window.clear();
            gameLoop.update(registry, deltaTime);
            eventBus.process();
            window.display();
        }
    }

    GameOverMenu::Result runGameOverMenu(Window& window, Registry& registry, FontManager& fontManager,
                                         ButtonSystem& buttonSystem, int finalScore, bool victory)
    {
        GameOverMenu gameOverMenu(fontManager, finalScore, victory);
        gameOverMenu.create(registry);

        g_running     = true;
        auto lastTime = std::chrono::steady_clock::now();

        while (window.isOpen() && g_running && !gameOverMenu.isDone()) {
            window.pollEvents([&](const Event& event) {
                if (event.type == EventType::Closed) {
                    g_running = false;
                    window.close();
                }
                gameOverMenu.handleEvent(registry, event);
                buttonSystem.handleEvent(registry, event);
            });

            auto currentTime                     = std::chrono::steady_clock::now();
            std::chrono::duration<float> elapsed = currentTime - lastTime;
            lastTime                             = currentTime;
            const float deltaTime                = std::min(elapsed.count(), 0.1F);

            window.setColorFilter(g_colorFilterMode);

            buttonSystem.update(registry, deltaTime);

            window.clear();
            gameOverMenu.render(registry, window);
            window.display();
        }

        GameOverMenu::Result result = gameOverMenu.getResult();
        gameOverMenu.destroy(registry);
        return result;
    }

} // namespace

GameSessionResult runGameSession(std::uint32_t localPlayerId, RoomType gameMode, Window& window,
                                 const ClientOptions& options, const IpEndpoint& serverEndpoint, NetPipelines& net,
                                 InputBuffer& inputBuffer, TextureManager& textureManager, FontManager& fontManager,
                                 std::string& errorMessage, ThreadSafeQueue<NotificationData>& broadcastQueue)
{
    GraphicsFactory graphicsFactory;
    SoundManager soundManager;
    InputMapper mapper;
    mapper.setBindings(g_keyBindings);
    SoundManager::setGlobalVolume(g_musicVolume);

    Registry registry;
    EntityTypeRegistry typeRegistry;
    AssetManifest manifest        = loadManifest();
    AnimationAtlas animationAtlas = AnimationManifest::loadFromFile("client/assets/animations.json");
    AnimationRegistry& animations = animationAtlas.clips;
    AnimationLabels animationLabels{animationAtlas.labels};
    LevelState levelState{};

    if (!fontManager.has("score_font")) {
        fontManager.load("score_font", "client/assets/fonts/ui.ttf");
    }

    (void) options;
    bool serverLost = false;

    if (g_isRoomHost && g_expectedPlayerCount > 0) {
        Logger::instance().info("[RunClientFlow] Sending expected player count: " +
                                std::to_string(g_expectedPlayerCount));
        PacketHeader hdr{};
        hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
        hdr.messageType = static_cast<std::uint8_t>(MessageType::RoomSetPlayerCount);
        hdr.sequenceId  = 0;
        hdr.payloadSize = 1;

        auto encoded = hdr.encode();
        std::vector<std::uint8_t> packet(encoded.begin(), encoded.end());
        packet.push_back(g_expectedPlayerCount);

        auto crc = PacketHeader::crc32(packet.data(), packet.size());
        packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

        net.socket->sendTo(packet.data(), packet.size(), serverEndpoint);
    }

    sendClientReady(serverEndpoint, *net.socket);

    if (!window.isOpen()) {
        return GameSessionResult{false, false, std::nullopt};
    }

    registerEntityTypes(typeRegistry, textureManager, manifest);

    EventBus eventBus;
    GameState gameState;

    eventBus.subscribe<GameOverEvent>([&](const GameOverEvent& event) {
        std::cout << ">>> CLIENT RECEIVED GAMEOVER EVENT. Victory: " << event.victory << std::endl;
        Logger::instance().info("[RunClientFlow] GameOverEvent received. Victory: " +
                                (event.victory ? std::string("true") : std::string("false")));
        gameState.gameOverTriggered = true;
        gameState.finalScore        = event.finalScore;
        gameState.victory           = event.victory;
        g_running                   = false;
    });

    GameLoop gameLoop;
    std::uint32_t inputSequence = 0;
    float playerPosX            = 0.0F;
    float playerPosY            = 0.0F;

    configureSystems(localPlayerId, gameMode, gameLoop, net, typeRegistry, manifest, textureManager, animations,
                     animationLabels, levelState, inputBuffer, mapper, inputSequence, playerPosX, playerPosY, window,
                     fontManager, eventBus, graphicsFactory, soundManager, broadcastQueue);

    ButtonSystem buttonSystem(window, fontManager);

    bool disconnected = false;
    runMainGameLoop(window, gameLoop, registry, eventBus, mapper, buttonSystem, net, errorMessage, disconnected,
                    serverLost, broadcastQueue, serverEndpoint);

    sendDisconnectPacket(serverEndpoint, net);
    gameLoop.stop();

    if (gameState.gameOverTriggered) {
        auto result =
            runGameOverMenu(window, registry, fontManager, buttonSystem, gameState.finalScore, gameState.victory);
        if (result == GameOverMenu::Result::Retry) {
            return GameSessionResult{true, serverLost, std::nullopt};
        }
    }

    bool retry = disconnected || !errorMessage.empty();
    return GameSessionResult{retry, serverLost, (retry ? std::nullopt : std::make_optional(0))};
}
