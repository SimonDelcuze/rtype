#include "Main.hpp"

#include "Logger.hpp"

#include <string>

int main(int argc, char* argv[])
{
    bool verbose = false;
    bool network = false;

    for (int i = 1; i < argc; ++i) {
        const std::string argument(argv[i]);
        if (argument == "--verbose" || argument == "-v") {
            verbose = true;
        }
        if (argument == "--network" || argument == "-n") {
            network = true;
        }
    }

    Logger::instance().setVerbose(verbose);
    Logger::instance().loadTagConfig("server.log.config");

    if (verbose) {
        Logger::instance().info("[Net] Verbose logging enabled");
    }

    run_server(network);
    return 0;
}
