#include "Logger.hpp"
#include "Main.hpp"
#include "server/ServerRunner.hpp"

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

void run_server(bool enableTui)
{
    std::signal(SIGINT, signalHandler);
    ServerApp app(50010, g_running, enableTui);
    if (!app.start()) {
        Logger::instance().error("[Net] Failed to start server");
        return;
    }
    app.run();
    app.stop();
}
