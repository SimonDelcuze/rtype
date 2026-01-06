#include "Main.hpp"

#include "Logger.hpp"

#include <string>

int main(int argc, char* argv[])
{
    bool verbose = false;
    bool network = false;
    bool admin   = false;

    for (int i = 1; i < argc; ++i) {
        const std::string argument(argv[i]);
        if (argument == "--verbose" || argument == "-v") {
            verbose = true;
        }
        if (argument == "--network" || argument == "-n") {
            network = true;
        }
        if (argument == "--admin" || argument == "-a") {
            admin = true;
        }
    }

    Logger::instance().setVerbose(verbose);
    Logger::instance().loadTagConfig("server.log.config");

    if (verbose) {
        Logger::instance().info("[Net] Verbose logging enabled");
    }

    run_server(network, admin);
    return 0;
}
