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
#include "network/LevelInitData.hpp"
#include "network/NetworkMessageHandler.hpp"
#include "network/NetworkReceiver.hpp"
#include "network/NetworkSender.hpp"
#include "scheduler/GameLoop.hpp"
#include "systems/AnimationSystem.hpp"
#include "systems/LevelInitSystem.hpp"
#include "systems/NetworkMessageSystem.hpp"
#include "systems/RenderSystem.hpp"
#include "systems/ReplicationSystem.hpp"

#include <SFML/Window/VideoMode.hpp>
#include <iostream>
#include <memory>

namespace
{
    struct NetPipelines
    {
        ThreadSafeQueue<std::vector<std::uint8_t>> raw;
        ThreadSafeQueue<SnapshotParseResult> parsed;
        ThreadSafeQueue<LevelInitData> levelInit;
        std::unique_ptr<NetworkReceiver> receiver;
        std::unique_ptr<NetworkMessageHandler> handler;
        std::unique_ptr<NetworkSender> sender;
    };

    bool startReceiver(NetPipelines& net, std::uint16_t port)
    {
        net.receiver = std::make_unique<NetworkReceiver>(
            IpEndpoint::v4(0, 0, 0, 0, port),
            [&](std::vector<std::uint8_t>&& packet) { net.raw.push(std::move(packet)); });
        if (!net.receiver->start()) {
            std::cerr << "Failed to start NetworkReceiver on port " << port << '\n';
            return false;
        }
        net.handler = std::make_unique<NetworkMessageHandler>(net.raw, net.parsed, net.levelInit);
        return true;
    }

    bool startSender(NetPipelines& net, InputBuffer& buffer, std::uint32_t playerId, const IpEndpoint& remote)
    {
        net.sender = std::make_unique<NetworkSender>(
            buffer, remote, playerId, std::chrono::milliseconds(16), IpEndpoint::v4(0, 0, 0, 0, 0),
            [](const IError& err) { std::cerr << "NetworkSender error: " << err.what() << '\n'; });
        if (!net.sender->start()) {
            std::cerr << "Failed to start NetworkSender\n";
            return false;
        }
        return true;
    }

    EntityId createPlayer(Registry& registry, TextureManager& textures)
    {
        EntityId player     = registry.createEntity();
        const auto& texture = textures.load("player", "client/assets/sprites/r-typesheet1.png");
        auto& sprite        = registry.emplace<SpriteComponent>(player, SpriteComponent(texture));
        sprite.setFrameSize(32, 32, 8);
        registry.emplace<TransformComponent>(player, TransformComponent::create(200.0F, 200.0F));
        registry.emplace<AnimationComponent>(player, AnimationComponent::create(8, 0.1F));
        registry.emplace<LayerComponent>(player, LayerComponent::create(0));
        return player;
    }
} // namespace

int main()
{
    Window window(sf::VideoMode({1280u, 720u}), "R-Type", true);

    TextureManager textureManager;
    Registry registry;
    InputBuffer inputBuffer;
    NetPipelines net;
    EntityTypeRegistry typeRegistry;
    LevelState levelState{};

    AssetManifest manifest;
    try {
        manifest = AssetManifest::fromFile("client/assets/assets.json");
    } catch (const std::exception& e) {
        std::cerr << "Failed to load asset manifest: " << e.what() << '\n';
    }

    startReceiver(net, 50000);
    startSender(net, inputBuffer, 1, IpEndpoint::v4(127, 0, 0, 1, 50001));

    try {
        createPlayer(registry, textureManager);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load player texture: " << e.what() << '\n';
        return 1;
    }

    GameLoop gameLoop;
    std::uint32_t inputSequence = 0;
    float playerPosX            = 0.0F;
    float playerPosY            = 0.0F;
    InputMapper mapper;
    gameLoop.addSystem(std::make_shared<InputSystem>(inputBuffer, mapper, inputSequence, playerPosX, playerPosY));
    gameLoop.addSystem(std::make_shared<ReplicationSystem>(net.parsed));
    gameLoop.addSystem(
        std::make_shared<LevelInitSystem>(net.levelInit, typeRegistry, manifest, textureManager, levelState));
    gameLoop.addSystem(std::make_shared<AnimationSystem>());
    gameLoop.addSystem(std::make_shared<NetworkMessageSystem>(*net.handler));
    gameLoop.addSystem(std::make_shared<RenderSystem>(window));

    int rc = gameLoop.run(window, registry);
    if (net.receiver) {
        net.receiver->stop();
    }
    if (net.sender) {
        net.sender->stop();
    }
    return rc;
}
