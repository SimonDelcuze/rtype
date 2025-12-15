#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "animation/AnimationManifest.hpp"
#include "ecs/Registry.hpp"
#include "events/EventBus.hpp"
#include "events/GameEvents.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
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

#include <SFML/System/Clock.hpp>

std::optional<IpEndpoint> resolveServerEndpoint(const ClientOptions& options, Window& window, FontManager& fontManager,
                                                TextureManager& textureManager, std::string& errorMessage)
{
    if (options.useDefault) {
        Logger::instance().info("Using default server: 127.0.0.1:50010");
        return IpEndpoint::v4(127, 0, 0, 1, 50010);
    }
    return showConnectionMenu(window, fontManager, textureManager, errorMessage);
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

GameSessionResult runGameSession(Window& window, const ClientOptions& options, const IpEndpoint& serverEndpoint,
                                 NetPipelines& net, InputBuffer& inputBuffer, TextureManager& textureManager,
                                 FontManager& fontManager)
{
    Registry registry;
    EntityTypeRegistry typeRegistry;
    AssetManifest manifest = loadManifest();

    AnimationAtlas animationAtlas = AnimationManifest::loadFromFile("client/assets/animations.json");
    AnimationRegistry& animations = animationAtlas.clips;
    AnimationLabels animationLabels{animationAtlas.labels};
    LevelState levelState{};

    if (options.useDefault) {
        Logger::instance().info("Default mode: auto-sending CLIENT_READY");
        sendClientReady(serverEndpoint, *net.socket);
    } else if (!runWaitingRoom(window, net, serverEndpoint)) {
        return GameSessionResult{false, std::nullopt};
    }

    if (!window.isOpen()) {
        return GameSessionResult{false, std::nullopt};
    }

    registerEntityTypes(typeRegistry, textureManager, manifest);

    EventBus eventBus;
    bool gameOverTriggered = false;
    int finalScore         = 0;
    bool victory           = false;

    eventBus.subscribe<GameOverEvent>([&](const GameOverEvent& event) {
        Logger::instance().info("[GameSession] Game Over event received - victory=" + std::to_string(event.victory));
        gameOverTriggered = true;
        finalScore        = event.finalScore;
        victory           = event.victory;
        g_running         = false;
    });

    GameLoop gameLoop;
    std::uint32_t inputSequence = 0;
    float playerPosX            = 0.0F;
    float playerPosY            = 0.0F;
    InputMapper mapper;

    configureSystems(gameLoop, net, typeRegistry, manifest, textureManager, animations, animationLabels, levelState,
                     inputBuffer, mapper, inputSequence, playerPosX, playerPosY, window, fontManager, eventBus);

    ButtonSystem buttonSystem(window, fontManager);

    auto onEvent = [&](const sf::Event& event) {
        mapper.handleEvent(event);
        buttonSystem.handleEvent(registry, event);
    };

    sf::Clock gameClock;
    while (window.isOpen() && g_running) {
        window.pollEvents([&](const sf::Event& event) {
            onEvent(event);
            if (event.is<sf::Event::Closed>()) {
                g_running = false;
                window.close();
            }
        });

        const float deltaTime = std::min(gameClock.restart().asSeconds(), 0.1F);

        window.clear();
        gameLoop.update(registry, deltaTime);
        eventBus.process();
        window.display();
    }

    if (net.socket && g_running == false) {
        PacketHeader header{};
        header.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
        header.messageType = static_cast<std::uint8_t>(MessageType::ClientDisconnect);
        header.sequenceId  = 0;
        header.payloadSize = 0;
        auto packet        = header.encode();
        net.socket->sendTo(packet.data(), packet.size(), serverEndpoint);
        Logger::instance().info("Sent CLIENT_DISCONNECT to server");
    }

    gameLoop.stop();

    Logger::instance().info("[GameSession] Game loop ended. gameOverTriggered=" + std::to_string(gameOverTriggered));

    if (gameOverTriggered) {
        Logger::instance().info("[GameSession] Showing Game Over menu");

        GameOverMenu gameOverMenu(fontManager, finalScore, victory);
        gameOverMenu.create(registry);

        g_running = true;
        sf::Clock clock;

        Logger::instance().info(
            "[GameSession] Starting Game Over menu loop - window.isOpen()=" + std::to_string(window.isOpen()) +
            " g_running=" + std::to_string(g_running) + " isDone=" + std::to_string(gameOverMenu.isDone()));

        int frameCount = 0;
        while (window.isOpen() && g_running && !gameOverMenu.isDone()) {
            if (frameCount++ % 60 == 0) {
                Logger::instance().info("[GameSession] Menu loop frame " + std::to_string(frameCount));
            }

            window.pollEvents([&](const sf::Event& event) {
                if (event.is<sf::Event::Closed>()) {
                    g_running = false;
                    window.close();
                }
                gameOverMenu.handleEvent(registry, event);
                buttonSystem.handleEvent(registry, event);
            });

            const float deltaTime = std::min(clock.restart().asSeconds(), 0.1F);
            buttonSystem.update(registry, deltaTime);

            window.clear();
            gameOverMenu.render(registry, window);
            window.display();
        }

        Logger::instance().info("[GameSession] Exited menu loop - window.isOpen()=" + std::to_string(window.isOpen()) +
                                " g_running=" + std::to_string(g_running) + " isDone=" +
                                std::to_string(gameOverMenu.isDone()) + " frameCount=" + std::to_string(frameCount));

        gameOverMenu.destroy(registry);

        if (gameOverMenu.getResult() == GameOverMenu::Result::Retry) {
            Logger::instance().info("[GameSession] User chose to retry - requesting reconnection");
            return GameSessionResult{true, std::nullopt};
        }
    }

    return GameSessionResult{false, 0};
}
