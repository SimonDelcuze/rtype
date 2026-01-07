#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "runtime/MenuMusic.hpp"

ClientLoopResult runClientIteration(const ClientOptions& options, Window& window, FontManager& fontManager,
                                    TextureManager& textureManager, std::string& errorMessage)
{
    auto serverEndpoint = resolveServerEndpoint(options, window, fontManager, textureManager, errorMessage);
    if (!serverEndpoint) {
        return ClientLoopResult{false, 0};
    }

    InputBuffer inputBuffer;
    NetPipelines net;
    std::atomic<bool> handshakeDone{false};
    std::thread welcomeThread;

    if (!setupNetwork(net, inputBuffer, *serverEndpoint, handshakeDone, welcomeThread)) {
        errorMessage = "Failed to setup network";
        return ClientLoopResult{true, std::nullopt};
    }

    auto joinResult = waitForJoinResponse(window, net);
    if (joinResult != JoinResult::Accepted) {
        auto exitCode = handleJoinFailure(joinResult, window, options, net, welcomeThread, handshakeDone, errorMessage);
        return ClientLoopResult{exitCode.has_value() ? false : true, exitCode};
    }

    stopLauncherMusic();
    auto gameResult = runGameSession(window, options, *serverEndpoint, net, inputBuffer, textureManager, fontManager, errorMessage);
    stopNetwork(net, welcomeThread, handshakeDone);

    if (gameResult.retry) {
        Logger::instance().info("User requested retry - restarting client loop");
        g_running = true;
        return ClientLoopResult{true, std::nullopt};
    }

    Logger::instance().info("R-Type Client shutting down");
    return ClientLoopResult{false, gameResult.exitCode};
}
