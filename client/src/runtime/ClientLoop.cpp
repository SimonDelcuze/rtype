#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "auth/AuthResult.hpp"
#include "runtime/MenuMusic.hpp"

ClientLoopResult runClientIteration(const ClientOptions& options, Window& window, FontManager& fontManager,
                                    TextureManager& textureManager, std::string& errorMessage,
                                    ThreadSafeQueue<NotificationData>& broadcastQueue,
                                    std::optional<IpEndpoint>& lastLobbyEndpoint, AuthResult* preservedAuth)
{
    NetPipelines net;
    std::uint32_t userId = 0;
    auto resolution = resolveServerEndpoint(options, window, fontManager, textureManager, errorMessage, broadcastQueue,
                                            lastLobbyEndpoint, userId, preservedAuth);
    if (!resolution) {
        return ClientLoopResult{false, 0, std::nullopt, std::nullopt};
    }
    const auto& [serverEndpoint, gameMode] = *resolution;

    InputBuffer inputBuffer;
    std::atomic<bool> handshakeDone{false};
    std::thread welcomeThread;

    if (!setupNetwork(net, inputBuffer, serverEndpoint, handshakeDone, welcomeThread, &broadcastQueue)) {
        broadcastQueue.push(NotificationData{"Failed to setup network", 5.0F});
        return ClientLoopResult{true, std::nullopt, std::nullopt, std::nullopt};
    }

    JoinResult joinResult = waitForJoinResponse(window, net);
    if (joinResult != JoinResult::Accepted) {
        if (joinResult == JoinResult::Timeout) {
            lastLobbyEndpoint.reset();
        }
        auto exitCode = handleJoinFailure(joinResult, window, options, net, welcomeThread, handshakeDone, errorMessage,
                                          broadcastQueue);
        stopNetwork(net, welcomeThread, handshakeDone);
        return ClientLoopResult{exitCode.has_value() ? false : true, exitCode, std::nullopt, std::nullopt};
    }

    std::uint32_t receivedPlayerId = net.receivedPlayerId.load();
    if (receivedPlayerId != 0 && net.sender) {
        net.sender->setPlayerId(receivedPlayerId);
        Logger::instance().info("[ClientLoop] Updated NetworkSender with playerId: " +
                                std::to_string(receivedPlayerId));
    }

    stopLauncherMusic();
    auto gameResult =
        runGameSession(receivedPlayerId != 0 ? receivedPlayerId : userId, gameMode, window, options, serverEndpoint,
                       net, inputBuffer, textureManager, fontManager, errorMessage, broadcastQueue);

    stopNetwork(net, welcomeThread, handshakeDone);

    if (gameResult.serverLost) {
        Logger::instance().info("Server connection lost - clearing lobby persistence");
        lastLobbyEndpoint.reset();
        errorMessage = "Connection lost to server";
    }

    if (gameResult.retry) {
        Logger::instance().info("User requested retry - preserving authentication and returning to lobby");
        g_running = true;
        if (preservedAuth != nullptr && preservedAuth->userId == 0) {
            preservedAuth->userId = userId;
        }
        return ClientLoopResult{true, std::nullopt, std::nullopt, std::nullopt};
    }

    Logger::instance().info("R-Type Client shutting down");
    return ClientLoopResult{false, gameResult.exitCode, std::nullopt, std::nullopt};
}
