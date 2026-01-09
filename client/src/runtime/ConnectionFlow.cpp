#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/abstraction/Event.hpp"
#include "network/EndpointParser.hpp"
#include "runtime/MenuMusic.hpp"
#include "systems/ButtonSystem.hpp"
#include "systems/HUDSystem.hpp"
#include "systems/RenderSystem.hpp"
#include "ui/ConnectionMenu.hpp"
#include "ui/LobbyMenu.hpp"
#include "ui/MenuRunner.hpp"
#include "ui/SettingsMenu.hpp"
#include "ui/WaitingRoomMenu.hpp"

#include <chrono>

std::optional<IpEndpoint> showConnectionMenu(Window& window, FontManager& fontManager, TextureManager& textureManager,
                                             std::string& errorMessage,
                                             ThreadSafeQueue<NotificationData>& broadcastQueue)
{
    startLauncherMusic(g_musicVolume);
    MenuRunner runner(window, fontManager, textureManager, g_running, broadcastQueue);

    while (window.isOpen()) {
        auto result = runner.runAndGetResult<ConnectionMenu>(errorMessage);
        if (!errorMessage.empty()) {
            broadcastQueue.push(NotificationData{errorMessage, 5.0F});
        }
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

bool verifyLobbyConnection(const IpEndpoint& lobbyEndpoint, std::string& errorMessage,
                           const std::atomic<bool>& runningFlag, ThreadSafeQueue<NotificationData>& broadcastQueue)
{
    LobbyConnection conn(lobbyEndpoint, runningFlag);
    if (!conn.connect()) {
        errorMessage = "Failed to open socket";
        broadcastQueue.push(NotificationData{errorMessage, 5.0F});
        return false;
    }
    if (!conn.ping()) {
        errorMessage = "Server not responding";
        broadcastQueue.push(NotificationData{errorMessage, 5.0F});
        return false;
    }
    return true;
}

std::optional<IpEndpoint> showLobbyMenuAndGetGameEndpoint(Window& window, const IpEndpoint& lobbyEndpoint,
                                                          FontManager& fontManager, TextureManager& textureManager,
                                                          ThreadSafeQueue<NotificationData>& broadcastQueue)
{
    MenuRunner runner(window, fontManager, textureManager, g_running, broadcastQueue);

    auto result = runner.runAndGetResult<LobbyMenu>(lobbyEndpoint, broadcastQueue, g_running);

    if (!window.isOpen())
        return std::nullopt;

    if (result.backRequested || result.exitRequested) {
        return std::nullopt;
    }

    if (result.success) {
        Logger::instance().info("[ConnectionFlow] Lobby returned game endpoint: port " +
                                std::to_string(result.gamePort) + " with " +
                                std::to_string(result.expectedPlayerCount) + " expected players");
        g_isRoomHost = result.isHost;
        g_expectedPlayerCount = result.expectedPlayerCount;
        return IpEndpoint::v4(lobbyEndpoint.addr[0], lobbyEndpoint.addr[1], lobbyEndpoint.addr[2],
                              lobbyEndpoint.addr[3], result.gamePort);
    }

    return std::nullopt;
}

std::optional<IpEndpoint> resolveServerEndpoint(const ClientOptions& options, Window& window, FontManager& fontManager,
                                                TextureManager& textureManager, std::string& errorMessage,
                                                ThreadSafeQueue<NotificationData>& broadcastQueue,
                                                std::optional<IpEndpoint>& lastLobbyEndpoint)
{
    if (options.useDefault) {
        Logger::instance().info("Using default lobby: 127.0.0.1:50010");
        auto lobbyEp = IpEndpoint::v4(127, 0, 0, 1, 50010);
        return showLobbyMenuAndGetGameEndpoint(window, lobbyEp, fontManager, textureManager, broadcastQueue);
    }

    if (options.serverIp.has_value() && options.serverPort.has_value()) {
        return parseEndpoint(*options.serverIp, std::to_string(*options.serverPort));
    }

    if (lastLobbyEndpoint.has_value()) {
        Logger::instance().info(
            "[Redirection] Returning to last lobby: " + std::to_string(lastLobbyEndpoint->addr[0]) + "." +
            std::to_string(lastLobbyEndpoint->addr[1]) + "." + std::to_string(lastLobbyEndpoint->addr[2]) + "." +
            std::to_string(lastLobbyEndpoint->addr[3]) + ":" + std::to_string(lastLobbyEndpoint->port));
        if (verifyLobbyConnection(*lastLobbyEndpoint, errorMessage, g_running, broadcastQueue)) {
            auto gameEp = showLobbyMenuAndGetGameEndpoint(window, *lastLobbyEndpoint, fontManager, textureManager,
                                                          broadcastQueue);
            if (gameEp.has_value()) {
                return gameEp;
            }
        } else {
            errorMessage.clear();
        }
        lastLobbyEndpoint.reset();
    }

    while (window.isOpen()) {
        auto lobbyEp = showConnectionMenu(window, fontManager, textureManager, errorMessage, broadcastQueue);
        if (!lobbyEp.has_value()) {
            return std::nullopt;
        }

        if (!verifyLobbyConnection(*lobbyEp, errorMessage, g_running, broadcastQueue)) {
            continue;
        }

        auto gameEp = showLobbyMenuAndGetGameEndpoint(window, *lobbyEp, fontManager, textureManager, broadcastQueue);
        if (gameEp.has_value()) {
            lastLobbyEndpoint = *lobbyEp;
            return gameEp;
        }
    }

    return std::nullopt;
}

JoinResult waitForJoinResponse(Window& window, NetPipelines& net, float timeoutSeconds)
{
    auto startTime = std::chrono::steady_clock::now();
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

        auto currentTime                     = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = currentTime - startTime;
        if (elapsed.count() > timeoutSeconds) {
            Logger::instance().warn("Timeout waiting for server response");
            return JoinResult::Timeout;
        }

        window.pollEvents([&](const Event& event) {
            if (event.type == EventType::Closed) {
                window.close();
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    return JoinResult::Timeout;
}

bool runWaitingRoom(Window& window, NetPipelines& net, const IpEndpoint& serverEp, std::string& errorMessage,
                    ThreadSafeQueue<NotificationData>& broadcastQueue, bool& serverLost)
{
    FontManager fontManager;
    TextureManager textureManager;
    Registry registry;
    (void) errorMessage;

    if (!fontManager.has("ui"))
        fontManager.load("ui", "client/assets/fonts/ui.ttf");

    WaitingRoomMenu menu(fontManager, textureManager, *net.socket, serverEp, net.allReady, net.countdownValue,
                         net.gameStartReceived);
    menu.create(registry);

    ButtonSystem buttonSystem(window, fontManager);
    HUDSystem hudSystem(window, fontManager, textureManager);
    RenderSystem renderSystem(window);
    NotificationSystem notificationSystem(window, fontManager, broadcastQueue);

    auto lastTime = std::chrono::steady_clock::now();

    while (window.isOpen() && !menu.isDone() && g_running) {
        if (net.handler)
            net.handler->poll();

        std::string disconnectMsg;
        if (net.disconnectEvents.tryPop(disconnectMsg)) {
            Logger::instance().warn("[Net] Disconnected from waiting room: " + disconnectMsg);
            if (disconnectMsg == "Server disconnected" || disconnectMsg == "Server timeout") {
                serverLost = true;
            }
            return false;
        }

        auto currentTime                     = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastTime;
        lastTime                             = currentTime;
        float dt                             = std::min(elapsed.count(), 0.1F);

        window.pollEvents([&](const Event& event) {
            if (event.type == EventType::Closed) {
                window.close();
                return;
            }
            buttonSystem.handleEvent(registry, event);
            menu.handleEvent(registry, event);
        });

        menu.update(registry, dt);

        window.clear(Color{30, 30, 40});

        renderSystem.update(registry, dt);

        buttonSystem.update(registry, dt);
        hudSystem.update(registry, dt);
        menu.render(registry, window);
        notificationSystem.update(registry, dt);
        window.display();
    }

    menu.destroy(registry);
    return menu.getResult(registry).ready || menu.isDone();
}
