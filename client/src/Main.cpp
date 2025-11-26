#include "components/AnimationComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "network/NetworkReceiver.hpp"
#include "scheduler/GameLoop.hpp"
#include "systems/AnimationSystem.hpp"
#include "systems/RenderSystem.hpp"

#include <SFML/Window/VideoMode.hpp>
#include <iostream>
#include <memory>
#include <vector>

int main()
{
    Window window(sf::VideoMode({1280u, 720u}), "R-Type", true);

    TextureManager textureManager;
    Registry registry;
    ThreadSafeQueue<std::vector<std::uint8_t>> snapshotQueue;

    NetworkReceiver receiver(IpEndpoint::v4(0, 0, 0, 0, 50000),
                             [&](std::vector<std::uint8_t>&& packet) { snapshotQueue.push(std::move(packet)); });
    if (!receiver.start()) {
        std::cerr << "Failed to start NetworkReceiver\n";
    }

    EntityId player = registry.createEntity();

    try {
        const auto& texture = textureManager.load("player", "client/assets/sprites/r-typesheet1.png");
        auto& sprite        = registry.emplace<SpriteComponent>(player, SpriteComponent(texture));
        sprite.setFrameSize(32, 32, 8);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load player texture: " << e.what() << '\n';
        return 1;
    }

    registry.emplace<TransformComponent>(player, TransformComponent::create(200.0F, 200.0F));
    registry.emplace<AnimationComponent>(player, AnimationComponent::create(8, 0.1F));
    registry.emplace<LayerComponent>(player, LayerComponent::create(0));
    GameLoop gameLoop;
    gameLoop.addSystem(std::make_shared<AnimationSystem>());
    gameLoop.addSystem(std::make_shared<RenderSystem>(window));

    int rc = gameLoop.run(window, registry);
    receiver.stop();
    return rc;
}
