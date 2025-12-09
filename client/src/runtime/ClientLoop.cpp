#include "ClientRuntime.hpp"
#include "Logger.hpp"

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

    auto gameResult = runGameSession(window, options, *serverEndpoint, net, inputBuffer, textureManager);
    stopNetwork(net, welcomeThread, handshakeDone);
    Logger::instance().info("R-Type Client shutting down");

    if (!gameResult.has_value()) {
        return ClientLoopResult{false, 0};
    }
    return ClientLoopResult{false, gameResult};
}
