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
#include <thread>

namespace
{
    AssetManifest loadManifest()
    {
        AssetManifest manifest;
        try {
            manifest = AssetManifest::fromFile("client/assets/assets.json");
        } catch (const std::exception& e) {
            std::cerr << "Failed to load asset manifest: " << e.what() << '\n';
        }
        return manifest;
    }

    void registerType(EntityTypeRegistry& types, TextureManager& textures, const AssetManifest& manifest,
                      AnimationRegistry& animations, std::uint16_t typeId,
                      const std::string& textureId, std::uint8_t layer)
    {
        RenderTypeData data{};
        data.spriteId = textureId;
        if (animations.has(textureId)) {
            data.animation = animations.get(textureId);
            auto clip      = animations.get(textureId);
            if (clip && !clip->frames.empty()) {
                data.frameCount    = static_cast<std::uint8_t>(clip->frames.size());
                data.frameDuration = clip->frameTime;
                data.frameWidth    = static_cast<std::uint32_t>(clip->frames.front().width);
                data.frameHeight   = static_cast<std::uint32_t>(clip->frames.front().height);
                data.columns       = 1;
            }
        }
        auto tex = manifest.findTextureById(textureId);
        if (tex) {
            if (!textures.has(tex->id))
                textures.load(tex->id, "client/assets/" + tex->path);
            data.texture = textures.get(tex->id);
        } else {
            data.texture = &textures.getPlaceholder();
        }
        data.layer = layer;
        types.registerType(typeId, data);
    }

    void setupTypes(EntityTypeRegistry& types, TextureManager& textures, const AssetManifest& manifest,
                    AnimationRegistry& animations)
    {
        registerType(types, textures, manifest, animations, 1, "player_ship",
                     static_cast<std::uint8_t>(RenderLayer::Entities));
        registerType(types, textures, manifest, animations, 2, "enemy_ship",
                     static_cast<std::uint8_t>(RenderLayer::Entities));
        registerType(types, textures, manifest, animations, 3, "bullet",
                     static_cast<std::uint8_t>(RenderLayer::Entities));
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
}

int main()
{
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
    AssetManifest manifest   = loadManifest();
    IpEndpoint serverEp = IpEndpoint::v4(127, 0, 0, 1, 50010);
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
    int rc = gameLoop.run(window, registry);
    stopNetwork(net, welcomeThread, handshakeDone);
    return rc;
}
