#include "ClientConfig.hpp"
#include "ClientRuntime.hpp"
#include "Logger.hpp"

#include <csignal>

void signalHandler(int signum)
{
    (void) signum;
    Logger::instance().info("Received signal " + std::to_string(signum) + ", stopping...");
    g_running = false;
}

int main(int argc, char* argv[])
{
    ClientOptions options = parseOptions(argc, argv);
    std::signal(SIGINT, signalHandler);
    return runClient(options);
}
