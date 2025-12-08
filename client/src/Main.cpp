#include "animation/AnimationManifest.hpp"
#include "animation/AnimationRegistry.hpp"
#include "assets/AssetManifest.hpp"
#include "components/AnimationComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "input/InputBuffer.hpp"
#include "input/InputMapper.hpp"
#include "input/InputSystem.hpp"
#include "level/EntityTypeRegistry.hpp"
#include "level/LevelState.hpp"
#include "Logger.hpp"
#include "network/ClientInit.hpp"
#include "scheduler/GameLoop.hpp"
#include "systems/AnimationSystem.hpp"
#include "systems/BackgroundScrollSystem.hpp"
#include "systems/LevelInitSystem.hpp"
#include "systems/NetworkMessageSystem.hpp"
#include "systems/RenderSystem.hpp"
#include "systems/ReplicationSystem.hpp"

#include <SFML/Window/VideoMode.hpp>
#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace
{
    AssetManifest loadManifest()
    {
        AssetManifest manifest;
        try {
            manifest = AssetManifest::fromFile("client/assets/assets.json");
        } catch (const std::exception& e) {
            Logger::instance().error("Failed to load asset manifest: " + std::string(e.what()));
        }
        return manifest;
    }

    void setupTypes(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest,
                    AnimationRegistry&)
    {
        auto playerTexEntry = manifest.findTextureById("player_ship");
        if (playerTexEntry) {
            if (!textures.has(playerTexEntry->id)) {
                textures.load(playerTexEntry->id, "client/assets/" + playerTexEntry->path);
            }
            auto* tex = textures.get(playerTexEntry->id);
            if (tex != nullptr) {
                RenderTypeData data{};
                data.texture = tex;
                data.layer = 0;
                data.spriteId = "player_ship";
                auto size = tex->getSize();
                data.frameCount = 6;
                data.frameDuration = 0.08F;
                data.columns = 6;
                data.frameWidth = data.columns == 0 ? size.x : static_cast<std::uint32_t>(size.x / data.columns);
                data.frameHeight = 14;
                registry.registerType(1, data);
            }
        }
        auto enemyTexEntry = manifest.findTextureById("enemy_ship");
        if (enemyTexEntry) {
            if (!textures.has(enemyTexEntry->id)) {
                textures.load(enemyTexEntry->id, "client/assets/" + enemyTexEntry->path);
            }
            auto* tex = textures.get(enemyTexEntry->id);
            if (tex != nullptr) {
                RenderTypeData data{};
                data.texture = tex;
                data.layer = 1;
                data.spriteId = "enemy_ship";
                registry.registerType(2, data);
            }
        }

        auto bulletTexEntry = manifest.findTextureById("bullet");
        if (bulletTexEntry) {
            if (!textures.has(bulletTexEntry->id)) {
                textures.load(bulletTexEntry->id, "client/assets/" + bulletTexEntry->path);
            }
            auto* tex = textures.get(bulletTexEntry->id);
            if (tex != nullptr) {
                RenderTypeData data{};
                data.texture = tex;
                data.layer = 1;
                data.spriteId = "bullet";
                registry.registerType(3, data);
            }
        }
    }

    void configureSystems(GameLoop& gameLoop, NetPipelines& net, EntityTypeRegistry& types,
                          const AssetManifest& manifest, TextureManager& textures, AnimationRegistry& animations,
                          AnimationLabels& labels, LevelState& levelState, InputBuffer& inputBuffer,
                          InputMapper& mapper, std::uint32_t& inputSequence, float& playerPosX, float& playerPosY,
                          Window& window)
    {
        gameLoop.addSystem(std::make_shared<InputSystem>(inputBuffer, mapper, inputSequence, playerPosX, playerPosY));
        gameLoop.addSystem(std::make_shared<NetworkMessageSystem>(*net.handler));
        gameLoop.addSystem(std::make_shared<LevelInitSystem>(net.levelInit, types, manifest, textures, animations,
                                                             labels, levelState));
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
} // namespace

int main(int argc, char* argv[])
{
    // Parse command line arguments
    bool verbose = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        }
    }

    // Initialize logger
    Logger::instance().setVerbose(verbose);
    Logger::instance().info("R-Type Client starting" + std::string(verbose ? " (verbose mode)" : ""));

    Window window(sf::VideoMode({1280u, 720u}), "R-Type", true);
    TextureManager textureManager;
    Registry registry;
    InputBuffer inputBuffer;
    NetPipelines net;
    EntityTypeRegistry typeRegistry;
    AnimationAtlas animationAtlas = AnimationManifest::loadFromFile("client/assets/animations.json");
    AnimationRegistry& animations = animationAtlas.clips;
    AnimationLabels animationLabels{animationAtlas.labels};
    LevelState levelState{};
    AssetManifest manifest = loadManifest();
    IpEndpoint serverEp    = IpEndpoint::v4(127, 0, 0, 1, 50010);
    std::atomic<bool> handshakeDone{false};
    std::thread welcomeThread;
    if (!setupNetwork(net, inputBuffer, serverEp, handshakeDone, welcomeThread))
        return 1;
    setupTypes(typeRegistry, textureManager, manifest, animations);

    GameLoop gameLoop;
    std::uint32_t inputSequence = 0;
    float playerPosX            = 0.0F;
    float playerPosY            = 0.0F;
    InputMapper mapper;
    configureSystems(gameLoop, net, typeRegistry, manifest, textureManager, animations, animationLabels, levelState,
                     inputBuffer, mapper, inputSequence, playerPosX, playerPosY, window);
    int rc = gameLoop.run(window, registry, net.socket.get(), &serverEp);
    stopNetwork(net, welcomeThread, handshakeDone);

    Logger::instance().info("R-Type Client shutting down");
    return rc;
}
