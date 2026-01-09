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
#include "network/LobbyConnection.hpp"
#include "network/PacketHeader.hpp"
#include "scheduler/ClientScheduler.hpp"
#include "scheduler/GameLoop.hpp"
#include "systems/ButtonSystem.hpp"
#include "systems/NotificationSystem.hpp"
#include "systems/RenderSystem.hpp"
#include "ui/ConnectionMenu.hpp"
#include "ui/GameOverMenu.hpp"

#include <chrono>

std::optional<AuthResult> showAuthenticationMenu(Window& window, FontManager& fontManager,
                                                 TextureManager& textureManager, LobbyConnection& lobbyConn,
                                                 ThreadSafeQueue<NotificationData>& broadcastQueue);

std::optional<IpEndpoint> resolveServerEndpoint(const ClientOptions& options, Window& window, FontManager& fontManager,
                                                TextureManager& textureManager, std::string& errorMessage,
                                                ThreadSafeQueue<NotificationData>& broadcastQueue,
                                                std::optional<IpEndpoint>& lastLobbyEndpoint)
{
    (void)lastLobbyEndpoint;

    static bool serverSelected = false;
    static IpEndpoint savedLobbyEp;
    static bool authenticated = false;
    static std::string authenticatedUsername;
    static std::unique_ptr<LobbyConnection> authenticatedConnection;

    if (!serverSelected) {
        Logger::instance().info("[Nav] Starting server selection flow");

        if (options.useDefault) {
            Logger::instance().info("Using default lobby: 127.0.0.1:50010");
            savedLobbyEp = IpEndpoint::v4(127, 0, 0, 1, 50010);
        } else {
            auto ep = showConnectionMenu(window, fontManager, textureManager, errorMessage, broadcastQueue);
            if (!ep.has_value()) {
                return std::nullopt;
            }
            savedLobbyEp = *ep;
        }
        serverSelected = true;
        Logger::instance().info("[Nav] Server selected");
    }

    while (window.isOpen() && !authenticated) {
        Logger::instance().info("[Auth] Starting authentication flow");

        authenticatedConnection = std::make_unique<LobbyConnection>(savedLobbyEp, g_running);

        auto authResult =
            showAuthenticationMenu(window, fontManager, textureManager, *authenticatedConnection, broadcastQueue);

        if (!authResult.has_value()) {
            Logger::instance().info("[Nav] Authentication cancelled, returning to server selection");
            serverSelected = false;
            authenticated  = false;
            authenticatedConnection.reset();

            if (options.useDefault) {
                return std::nullopt;
            }

            auto ep = showConnectionMenu(window, fontManager, textureManager, errorMessage, broadcastQueue);
            if (!ep.has_value()) {
                return std::nullopt;
            }
            savedLobbyEp   = *ep;
            serverSelected = true;
            continue;
        }

        Logger::instance().info("[Auth] User authenticated: " + authResult->username);
        authenticated         = true;
        authenticatedUsername = authResult->username;
    }

    while (window.isOpen()) {
        Logger::instance().info("[Nav] Showing lobby menu");

        auto gameEp = showLobbyMenuAndGetGameEndpoint(window, savedLobbyEp, fontManager, textureManager, broadcastQueue,
                                                      authenticatedConnection.get());

        if (gameEp.has_value()) {
            return gameEp;
        }

        Logger::instance().info("[Nav] Back from lobby, returning to login");
        authenticated = false;
        authenticatedConnection.reset();

        return resolveServerEndpoint(options, window, fontManager, textureManager, errorMessage, broadcastQueue, lastLobbyEndpoint);
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
                         bool& disconnected, bool& serverLost, ThreadSafeQueue<NotificationData>& broadcastQueue)
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

GameSessionResult runGameSession(Window& window, const ClientOptions& options, const IpEndpoint& serverEndpoint,
                                 NetPipelines& net, InputBuffer& inputBuffer, TextureManager& textureManager,
                                 FontManager& fontManager, std::string& errorMessage,
                                 ThreadSafeQueue<NotificationData>& broadcastQueue)
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

    bool serverLost = false;
    if (options.useDefault) {
        sendClientReady(serverEndpoint, *net.socket);
    } else if (!runWaitingRoom(window, net, serverEndpoint, errorMessage, broadcastQueue, serverLost)) {
        return GameSessionResult{true, serverLost, std::nullopt};
    }

    if (!window.isOpen()) {
        return GameSessionResult{false, false, std::nullopt};
    }

    registerEntityTypes(typeRegistry, textureManager, manifest);

    EventBus eventBus;
    GameState gameState;

    eventBus.subscribe<GameOverEvent>([&](const GameOverEvent& event) {
        gameState.gameOverTriggered = true;
        gameState.finalScore        = event.finalScore;
        gameState.victory           = event.victory;
        g_running                   = false;
    });

    GameLoop gameLoop;
    std::uint32_t inputSequence = 0;
    float playerPosX            = 0.0F;
    float playerPosY            = 0.0F;

    configureSystems(gameLoop, net, typeRegistry, manifest, textureManager, animations, animationLabels, levelState,
                     inputBuffer, mapper, inputSequence, playerPosX, playerPosY, window, fontManager, eventBus,
                     graphicsFactory, soundManager, broadcastQueue);

    ButtonSystem buttonSystem(window, fontManager);

    bool disconnected = false;
    runMainGameLoop(window, gameLoop, registry, eventBus, mapper, buttonSystem, net, errorMessage, disconnected,
                    serverLost, broadcastQueue);

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
