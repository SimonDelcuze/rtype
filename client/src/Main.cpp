#include "ClientConfig.hpp"
#include "ClientRuntime.hpp"
#include "Logger.hpp"

#include <csignal>

void signalHandler(int signum)
{
    (void) signum;
    g_forceExit = true;
    g_running   = false;
}

int main(int argc, char* argv[])
{
    Logger::instance().info("===== RTYPE CLIENT v2.0 WITH GAME OVER SYSTEM =====");
    ClientOptions options = parseOptions(argc, argv);
    std::signal(SIGINT, signalHandler);
    return runClient(options);
}
