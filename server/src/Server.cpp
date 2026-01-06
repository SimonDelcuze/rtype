#include "Logger.hpp"
#include "Main.hpp"
#include "lobby/LobbyServer.hpp"

#include <atomic>
#include <csignal>

namespace
{
    std::atomic<bool> g_running{true};

    void signalHandler(int signal)
    {
        if (signal == SIGINT || signal == SIGTERM)
            g_running = false;
    }
} // namespace

void run_server(bool enableTui, bool enableAdmin)
{
    std::signal(SIGINT, signalHandler);

    constexpr std::uint16_t kLobbyPort    = 50010;
    constexpr std::uint16_t kGameBasePort = 50100;
    constexpr std::uint32_t kMaxInstances = 10;

    LobbyServer server(kLobbyPort, kGameBasePort, kMaxInstances, g_running, enableTui, enableAdmin);

    if (!server.start()) {
        Logger::instance().error("[Net] Failed to start lobby server");
        return;
    }

    server.run();
    server.stop();
}
