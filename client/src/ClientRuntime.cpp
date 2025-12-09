#include "ClientRuntime.hpp"

#include "Logger.hpp"
#include "animation/AnimationManifest.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "input/InputSystem.hpp"
#include "level/EntityTypeSetup.hpp"
#include "network/EndpointParser.hpp"
#include "systems/AnimationSystem.hpp"
#include "systems/BackgroundScrollSystem.hpp"
#include "systems/LevelInitSystem.hpp"
#include "systems/NetworkMessageSystem.hpp"
#include "systems/RenderSystem.hpp"
#include "systems/ReplicationSystem.hpp"
#include "ui/ConnectionMenu.hpp"
#include "ui/MenuRunner.hpp"

#include <SFML/Window/VideoMode.hpp>

std::atomic<bool> g_running{true};

Window createMainWindow()
{
    return Window(sf::VideoMode({1280u, 720u}), "R-Type", true);
}

std::optional<IpEndpoint> selectServerEndpoint(Window& window, bool useDefault)
{
    if (useDefault) {
        Logger::instance().info("Using default server: 127.0.0.1:50010");
        return IpEndpoint::v4(127, 0, 0, 1, 50010);
    }

    FontManager fontManager;
    TextureManager textureManager;
    MenuRunner runner(window, fontManager, textureManager, g_running);

    auto result = runner.runAndGetResult<ConnectionMenu>();

    if (!window.isOpen())
        return std::nullopt;

    if (result.useDefault)
        return IpEndpoint::v4(127, 0, 0, 1, 50010);

    return parseEndpoint(result.ip, result.port);
}

AssetManifest loadManifest()
{
    try {
        return AssetManifest::fromFile("client/assets/assets.json");
    } catch (const std::exception& e) {
        Logger::instance().error("Failed to load asset manifest: " + std::string(e.what()));
        return AssetManifest{};
    }
}

void configureSystems(GameLoop& gameLoop, NetPipelines& net, EntityTypeRegistry& types, const AssetManifest& manifest,
                      TextureManager& textures, AnimationRegistry& animations, AnimationLabels& labels,
                      LevelState& levelState, InputBuffer& inputBuffer, InputMapper& mapper,
                      std::uint32_t& inputSequence, float& playerPosX, float& playerPosY, Window& window)
{
    gameLoop.addSystem(std::make_shared<InputSystem>(inputBuffer, mapper, inputSequence, playerPosX, playerPosY));
    gameLoop.addSystem(std::make_shared<NetworkMessageSystem>(*net.handler));
    gameLoop.addSystem(
        std::make_shared<LevelInitSystem>(net.levelInit, types, manifest, textures, animations, labels, levelState));
    gameLoop.addSystem(std::make_shared<ReplicationSystem>(net.parsed, types));
    gameLoop.addSystem(std::make_shared<AnimationSystem>());
    gameLoop.addSystem(std::make_shared<BackgroundScrollSystem>(window));
    gameLoop.addSystem(std::make_shared<RenderSystem>(window));
}

bool setupNetwork(NetPipelines& net, InputBuffer& inputBuffer, const IpEndpoint& serverEp,
                  std::atomic<bool>& handshakeDone, std::thread& welcomeThread)
{
    if (!startReceiver(net, 0, handshakeDone))
        return false;
    if (!startSender(net, inputBuffer, 1, serverEp))
        return false;

    welcomeThread = std::thread([&] {
        if (net.socket)
            sendWelcomeLoop(serverEp, handshakeDone, *net.socket);
    });
    return true;
}

void stopNetwork(NetPipelines& net, std::thread& welcomeThread, std::atomic<bool>& handshakeDone)
{
    handshakeDone.store(true);
    if (welcomeThread.joinable())
        welcomeThread.join();
    if (net.receiver)
        net.receiver->stop();
    if (net.sender)
        net.sender->stop();
}

int runClient(const ClientOptions& options)
{
    configureLogger(options.verbose);

    Window window       = createMainWindow();
    auto serverEndpoint = selectServerEndpoint(window, options.useDefault);
    if (!serverEndpoint)
        return 0;

    TextureManager textureManager;
    Registry registry;
    InputBuffer inputBuffer;
    NetPipelines net;
    EntityTypeRegistry typeRegistry;
    AssetManifest manifest = loadManifest();

    AnimationAtlas animationAtlas = AnimationManifest::loadFromFile("client/assets/animations.json");
    AnimationRegistry& animations = animationAtlas.clips;
    AnimationLabels animationLabels{animationAtlas.labels};
    LevelState levelState{};

    std::atomic<bool> handshakeDone{false};
    std::thread welcomeThread;
    if (!setupNetwork(net, inputBuffer, *serverEndpoint, handshakeDone, welcomeThread))
        return 1;

    registerEntityTypes(typeRegistry, textureManager, manifest);

    GameLoop gameLoop;
    std::uint32_t inputSequence = 0;
    float playerPosX            = 0.0F;
    float playerPosY            = 0.0F;
    InputMapper mapper;

    configureSystems(gameLoop, net, typeRegistry, manifest, textureManager, animations, animationLabels, levelState,
                     inputBuffer, mapper, inputSequence, playerPosX, playerPosY, window);

    int rc = gameLoop.run(window, registry, net.socket.get(), &*serverEndpoint, g_running);
    stopNetwork(net, welcomeThread, handshakeDone);
    Logger::instance().info("R-Type Client shutting down");
    return rc;
}
