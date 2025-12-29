#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "network/EndpointParser.hpp"
#include "runtime/MenuMusic.hpp"
#include "systems/ButtonSystem.hpp"
#include "systems/HUDSystem.hpp"
#include "ui/ConnectionMenu.hpp"
#include "ui/MenuRunner.hpp"
#include "ui/SettingsMenu.hpp"
#include "ui/WaitingRoomMenu.hpp"

#include <SFML/System/Clock.hpp>
#include <chrono>

std::optional<IpEndpoint> showConnectionMenu(Window& window, FontManager& fontManager, TextureManager& textureManager,
                                             std::string& errorMessage)
{
    startLauncherMusic(g_musicVolume);
    MenuRunner runner(window, fontManager, textureManager, g_running);

    while (window.isOpen()) {
        auto result = runner.runAndGetResult<ConnectionMenu>(errorMessage);
        errorMessage.clear();

        if (!window.isOpen())
            return std::nullopt;

        if (result.openSettings) {
            auto settingsResult = runner.runAndGetResult<SettingsMenu>(g_keyBindings, g_musicVolume);
            g_keyBindings       = settingsResult.bindings;
            g_musicVolume       = settingsResult.musicVolume;
            setLauncherMusicVolume(g_musicVolume);
            if (!window.isOpen())
                return std::nullopt;
            continue;
        }

        if (result.exitRequested) {
            window.close();
            return std::nullopt;
        }

        if (result.useDefault) {
            return IpEndpoint::v4(127, 0, 0, 1, 50010);
        }
        return parseEndpoint(result.ip, result.port);
    }

    return std::nullopt;
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

    while (window.isOpen()) {
        startLauncherMusic(g_musicVolume);
        auto result = runner.runAndGetResult<ConnectionMenu>();

        if (!window.isOpen())
            return std::nullopt;

        if (result.openSettings) {
            auto settingsResult = runner.runAndGetResult<SettingsMenu>(g_keyBindings, g_musicVolume);
            g_keyBindings       = settingsResult.bindings;
            g_musicVolume       = settingsResult.musicVolume;
            setLauncherMusicVolume(g_musicVolume);
            if (!window.isOpen())
                return std::nullopt;
            continue;
        }

        if (result.exitRequested) {
            window.close();
            return std::nullopt;
        }

        if (result.useDefault)
            return IpEndpoint::v4(127, 0, 0, 1, 50010);

        return parseEndpoint(result.ip, result.port);
    }

    return std::nullopt;
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
