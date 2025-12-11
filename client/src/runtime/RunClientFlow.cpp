#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "animation/AnimationManifest.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "input/InputMapper.hpp"
#include "level/EntityTypeSetup.hpp"
#include "level/LevelState.hpp"
#include "scheduler/GameLoop.hpp"
#include "ui/ConnectionMenu.hpp"

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

std::optional<int> runGameSession(Window& window, const ClientOptions& options, const IpEndpoint& serverEndpoint,
                                  NetPipelines& net, InputBuffer& inputBuffer, TextureManager& textureManager)
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
        return std::nullopt;
    }

    if (!window.isOpen()) {
        return std::nullopt;
    }

    registerEntityTypes(typeRegistry, textureManager, manifest);

    GameLoop gameLoop;
    std::uint32_t inputSequence = 0;
    float playerPosX            = 0.0F;
    float playerPosY            = 0.0F;
    InputMapper mapper;

    configureSystems(gameLoop, net, typeRegistry, manifest, textureManager, animations, animationLabels, levelState,
                     inputBuffer, mapper, inputSequence, playerPosX, playerPosY, window);

    auto onEvent = [&](const sf::Event& event) {
        mapper.handleEvent(event);
    };
    int rc = gameLoop.run(window, registry, net.socket.get(), &serverEndpoint, g_running, onEvent);
    return rc;
}
