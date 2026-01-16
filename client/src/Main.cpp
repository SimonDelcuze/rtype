#include "ClientConfig.hpp"
#include "ClientRuntime.hpp"
#include "Logger.hpp"

#include <csignal>

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#endif

void signalHandler(int signum)
{
    (void) signum;
    g_forceExit = true;
    g_running   = false;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    Logger::instance().info("===== RTYPE CLIENT v2.0 WITH GAME OVER SYSTEM =====");
    ClientOptions options = parseOptions(argc, argv);
    std::signal(SIGINT, signalHandler);
    return runClient(options);
}
