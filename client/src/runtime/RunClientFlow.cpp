#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "animation/AnimationManifest.hpp"
#include "audio/SoundManager.hpp"
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
#include "network/PacketHeader.hpp"
#include "scheduler/ClientScheduler.hpp"
#include "scheduler/GameLoop.hpp"
#include "systems/ButtonSystem.hpp"
#include "systems/RenderSystem.hpp"
#include "ui/ConnectionMenu.hpp"
#include "ui/GameOverMenu.hpp"

#include <chrono>

std::optional<IpEndpoint> resolveServerEndpoint(const ClientOptions& options, Window& window, FontManager& fontManager,
                                                TextureManager& textureManager, std::string& errorMessage,
                                                ThreadSafeQueue<std::string>& broadcastQueue)
{
    if (options.useDefault) {
        Logger::instance().info("Using default lobby: 127.0.0.1:50010");
        auto lobbyEp = IpEndpoint::v4(127, 0, 0, 1, 50010);
        return showLobbyMenuAndGetGameEndpoint(window, lobbyEp, fontManager, textureManager, broadcastQueue);
    }

    while (window.isOpen()) {
        auto lobbyEp = showConnectionMenu(window, fontManager, textureManager, errorMessage, broadcastQueue);
        if (!lobbyEp.has_value()) {
            return std::nullopt;
        }

        auto gameEp = showLobbyMenuAndGetGameEndpoint(window, *lobbyEp, fontManager, textureManager, broadcastQueue);
        if (gameEp.has_value()) {
            return gameEp;
        }
    }

    return std::nullopt;
}

std::optional<int> handleJoinFailure(JoinResult joinResult, Window& window, const ClientOptions& options,
                                     NetPipelines& net, std::thread& welcomeThread, std::atomic<bool>& handshakeDone,
                                     std::string& errorMessage)
{
    if (joinResult == JoinResult::Denied) {
        Logger::instance().error("Connection rejected - game already in progress!");
        stopNetwork(net, welcomeThread, handshakeDone);
        errorMessage = "Connection rejected - game in progress!";
        net.joinDenied.store(false);
        net.joinAccepted.store(false);
        if (options.useDefault) {
            showErrorMessage(window, errorMessage);
            return 1;
        }
        return std::nullopt;
    }

    if (joinResult == JoinResult::Timeout) {
        Logger::instance().error("Server did not respond - connection timeout");
        stopNetwork(net, welcomeThread, handshakeDone);
        errorMessage = "Server did not respond - timeout";
        if (options.useDefault) {
            showErrorMessage(window, errorMessage);
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
                         bool& disconnected)
    {
        auto onEvent = [&](const Event& event) {
            mapper.handleEvent(event);
            buttonSystem.handleEvent(registry, event);
        };
        (void) errorMessage;

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

            if (net.handler)
                net.handler->poll();

            std::string disconnectMsg;
            if (net.disconnectEvents.tryPop(disconnectMsg)) {
                Logger::instance().warn("[Net] Disconnected from server: " + disconnectMsg);
                disconnected   = true;
                sessionRunning = false;
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
                                 ThreadSafeQueue<std::string>& broadcastQueue)
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

    if (options.useDefault) {
        sendClientReady(serverEndpoint, *net.socket);
    } else if (!runWaitingRoom(window, net, serverEndpoint, errorMessage, broadcastQueue)) {
        return GameSessionResult{true, std::nullopt};
    }

    if (!window.isOpen()) {
        return GameSessionResult{false, std::nullopt};
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
    runMainGameLoop(window, gameLoop, registry, eventBus, mapper, buttonSystem, net, errorMessage, disconnected);

    sendDisconnectPacket(serverEndpoint, net);
    gameLoop.stop();

    if (gameState.gameOverTriggered) {
        auto result =
            runGameOverMenu(window, registry, fontManager, buttonSystem, gameState.finalScore, gameState.victory);
        if (result == GameOverMenu::Result::Retry) {
            return GameSessionResult{true, std::nullopt};
        }
    }

    bool retry = disconnected || !errorMessage.empty();
    return GameSessionResult{retry, (retry ? std::nullopt : std::make_optional(0))};
}
