#include "components/AnimationComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "input/InputBuffer.hpp"
#include "input/InputMapper.hpp"
#include "input/InputSystem.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "network/NetworkMessageHandler.hpp"
#include "network/NetworkReceiver.hpp"
#include "scheduler/GameLoop.hpp"
#include "systems/AnimationSystem.hpp"
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
    std::unique_ptr<NetworkReceiver> receiver;
    std::unique_ptr<NetworkMessageHandler> handler;
};

bool startReceiver(NetPipelines& net, std::uint16_t port)
{
    net.receiver = std::make_unique<NetworkReceiver>(
        IpEndpoint::v4(0, 0, 0, 0, port), [&](std::vector<std::uint8_t>&& packet) { net.raw.push(std::move(packet)); });
    if (!net.receiver->start()) {
        std::cerr << "Failed to start NetworkReceiver on port " << port << '\n';
        return false;
    }
    net.handler = std::make_unique<NetworkMessageHandler>(net.raw, net.parsed);
    return true;
}

EntityId createPlayer(Registry& registry, TextureManager& textures)
{
    EntityId player = registry.createEntity();
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
    startReceiver(net, 50000);

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
    gameLoop.addSystem(std::make_shared<AnimationSystem>());
    gameLoop.addSystem(std::make_shared<NetworkMessageSystem>(*net.handler));
    gameLoop.addSystem(std::make_shared<RenderSystem>(window));

    int rc = gameLoop.run(window, registry);
    if (net.receiver) {
        net.receiver->stop();
    }
    return rc;
}
