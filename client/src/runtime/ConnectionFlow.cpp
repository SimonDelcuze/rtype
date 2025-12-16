#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "network/EndpointParser.hpp"
#include "systems/ButtonSystem.hpp"
#include "systems/HUDSystem.hpp"
#include "systems/InputFieldSystem.hpp"
#include "ui/ConnectionMenu.hpp"
#include "ui/MenuRunner.hpp"
#include "ui/WaitingRoomMenu.hpp"

#include <SFML/System/Clock.hpp>
#include <chrono>

std::optional<IpEndpoint> showConnectionMenu(Window& window, FontManager& fontManager, TextureManager& textureManager,
                                             std::string& errorMessage)
{
    Registry menuRegistry;
    ConnectionMenu menu(fontManager, textureManager);
    menu.create(menuRegistry);

    if (!errorMessage.empty()) {
        menu.setError(menuRegistry, errorMessage);
        errorMessage.clear();
    }

    ButtonSystem buttonSystem(window, fontManager);
    HUDSystem hudSystem(window, fontManager, textureManager);
    InputFieldSystem inputFieldSystem(window, fontManager);
    sf::Clock clock;

    while (window.isOpen() && !menu.isDone() && g_running) {
        float dt = clock.restart().asSeconds();
        window.pollEvents([&](const sf::Event& event) {
            if (event.is<sf::Event::Closed>()) {
                window.close();
                return;
            }
            buttonSystem.handleEvent(menuRegistry, event);
            inputFieldSystem.handleEvent(menuRegistry, event);
        });

        buttonSystem.update(menuRegistry, dt);
        inputFieldSystem.update(menuRegistry, dt);

        window.clear(sf::Color(30, 30, 40));
        for (EntityId entity : menuRegistry.view<TransformComponent, SpriteComponent>()) {
            auto& transform = menuRegistry.get<TransformComponent>(entity);
            auto& sprite    = menuRegistry.get<SpriteComponent>(entity);
            if (sprite.hasSprite()) {
                auto* spr = const_cast<sf::Sprite*>(sprite.raw());
                spr->setPosition(sf::Vector2f{transform.x, transform.y});
                spr->setScale(sf::Vector2f{transform.scaleX, transform.scaleY});
                window.draw(*spr);
            }
        }
        buttonSystem.update(menuRegistry, dt);
        inputFieldSystem.update(menuRegistry, dt);
        hudSystem.update(menuRegistry, dt);
        window.display();
    }

    if (!window.isOpen()) {
        menu.destroy(menuRegistry);
        return std::nullopt;
    }

    auto result = menu.getResult(menuRegistry);
    menu.destroy(menuRegistry);

    if (result.useDefault) {
        return IpEndpoint::v4(127, 0, 0, 1, 50010);
    }
    return parseEndpoint(result.ip, result.port);
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

JoinResult waitForJoinResponse(Window& window, NetPipelines& net, float timeoutSeconds)
{
    sf::Clock clock;
    while (window.isOpen() && g_running) {
        if (net.handler)
            net.handler->poll();

        if (net.joinAccepted.load()) {
            Logger::instance().info("Join accepted by server");
            return JoinResult::Accepted;
        }
        if (net.joinDenied.load()) {
            Logger::instance().warn("Join denied by server - game already in progress");
            return JoinResult::Denied;
        }
        if (clock.getElapsedTime().asSeconds() > timeoutSeconds) {
            Logger::instance().warn("Timeout waiting for server response");
            return JoinResult::Timeout;
        }

        window.pollEvents([&](const sf::Event& event) {
            if (event.is<sf::Event::Closed>()) {
                window.close();
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    return JoinResult::Timeout;
}

bool runWaitingRoom(Window& window, NetPipelines& net, const IpEndpoint& serverEp)
{
    FontManager fontManager;
    TextureManager textureManager;
    Registry registry;

    if (!fontManager.has("ui"))
        fontManager.load("ui", "client/assets/fonts/ui.ttf");

    WaitingRoomMenu menu(fontManager, textureManager, *net.socket, serverEp, net.allReady, net.countdownValue,
                         net.gameStartReceived);
    menu.create(registry);

    ButtonSystem buttonSystem(window, fontManager);
    HUDSystem hudSystem(window, fontManager, textureManager);

    sf::Clock clock;

    while (window.isOpen() && !menu.isDone() && g_running) {
        float dt = clock.restart().asSeconds();

        window.pollEvents([&](const sf::Event& event) {
            if (event.is<sf::Event::Closed>()) {
                window.close();
                return;
            }
            buttonSystem.handleEvent(registry, event);
            menu.handleEvent(registry, event);
        });

        if (net.handler)
            net.handler->poll();

        menu.update(registry, dt);

        window.clear(sf::Color(30, 30, 40));

        for (EntityId entity : registry.view<TransformComponent, SpriteComponent>()) {
            auto& transform = registry.get<TransformComponent>(entity);
            auto& sprite    = registry.get<SpriteComponent>(entity);
            if (sprite.hasSprite()) {
                auto* spr = const_cast<sf::Sprite*>(sprite.raw());
                spr->setPosition(sf::Vector2f{transform.x, transform.y});
                spr->setScale(sf::Vector2f{transform.scaleX, transform.scaleY});
                window.draw(*spr);
            }
        }

        buttonSystem.update(registry, dt);
        hudSystem.update(registry, dt);
        menu.render(registry, window);
        window.display();
    }

    menu.destroy(registry);
    return menu.getResult(registry).ready || menu.isDone();
}
