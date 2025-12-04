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

    AssetManifest manifest;
    try {
        manifest = AssetManifest::fromFile("client/assets/assets.json");
    } catch (const std::exception& e) {
        std::cerr << "Failed to load asset manifest: " << e.what() << '\n';
    }

    IpEndpoint serverEp = IpEndpoint::v4(127, 0, 0, 1, 50010);
    std::atomic<bool> handshakeDone{false};
    if (!startReceiver(net, 50000, handshakeDone)) {
        return 1;
    }
    if (!startSender(net, inputBuffer, 1, serverEp)) {
        return 1;
    }
    std::thread welcomeThread([&] { sendWelcomeLoop(serverEp, handshakeDone); });

    // fallback registration in case no LevelInit arrives immediately
    auto registerType = [&](std::uint16_t typeId, const std::string& textureId, std::uint8_t layer) {
        RenderTypeData data{};
        data.spriteId = textureId;
        if (animations.has(textureId)) {
            data.animation = animations.get(textureId);
            auto clip      = animations.get(textureId);
            if (clip != nullptr && !clip->frames.empty()) {
                data.frameCount    = static_cast<std::uint8_t>(clip->frames.size());
                data.frameDuration = clip->frameTime;
                data.frameWidth    = static_cast<std::uint32_t>(clip->frames.front().width);
                data.frameHeight   = static_cast<std::uint32_t>(clip->frames.front().height);
                data.columns       = 1;
            }
        }
        auto tex = manifest.findTextureById(textureId);
        if (tex) {
            if (!textureManager.has(tex->id)) {
                textureManager.load(tex->id, "client/assets/" + tex->path);
            }
            data.texture = textureManager.get(tex->id);
        } else {
            data.texture = &textureManager.getPlaceholder();
        }
        data.layer = layer;
        typeRegistry.registerType(typeId, data);
    };

    registerType(1, "player_ship", static_cast<std::uint8_t>(RenderLayer::Entities));
    registerType(2, "enemy_ship", static_cast<std::uint8_t>(RenderLayer::Entities));
    registerType(3, "bullet", static_cast<std::uint8_t>(RenderLayer::Entities));

    GameLoop gameLoop;
    std::uint32_t inputSequence = 0;
    float playerPosX            = 0.0F;
    float playerPosY            = 0.0F;
    InputMapper mapper;
    gameLoop.addSystem(std::make_shared<InputSystem>(inputBuffer, mapper, inputSequence, playerPosX, playerPosY));
    gameLoop.addSystem(std::make_shared<NetworkMessageSystem>(*net.handler));
    gameLoop.addSystem(std::make_shared<LevelInitSystem>(net.levelInit, typeRegistry, manifest, textureManager,
                                                         animations, animationLabels, levelState));
    gameLoop.addSystem(std::make_shared<ReplicationSystem>(net.parsed, typeRegistry));
    gameLoop.addSystem(std::make_shared<AnimationSystem>());
    gameLoop.addSystem(std::make_shared<BackgroundScrollSystem>(window));
    gameLoop.addSystem(std::make_shared<RenderSystem>(window));

    int rc = gameLoop.run(window, registry);
    handshakeDone.store(true);
    if (welcomeThread.joinable()) {
        welcomeThread.join();
    }
    if (net.receiver) {
        net.receiver->stop();
    }
    if (net.sender) {
        net.sender->stop();
    }
    return rc;
}
